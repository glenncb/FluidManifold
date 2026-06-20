/*--------------------------------------------------------------------------------------------------------------*
 * Fluid Manifold PCB - Routines to control the FluidManifold module using RS-485 communication                  *
 *==============================================================================================================*
 *--------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/* main.cpp - main loop for FluidManifold PCB controller                */
/* Created: May 24, 2024, 2:41 AM (glenncb)                             */
/* Revised: May 24, 2024, 6:18 AM (glenncb) - ESP32 port                */
/*----------------------------------------------------------------------*/
// Resources used
// TMR0 - low frequency interrupt generator to blink LEDs or alternate colors (~.25Hz)
// UART0 - debug and programming serial port
// UART1 - Alicat/Clippard RS-485 serial communication
// UART2 - Control RS-485 modbus serial interface
// See pins.h for pin utilization and assignments of GPIOs

//----------------------------------------------------
// file: main.cpp
// language: C++
// Target processor: ESP32-S3-MINI-N4R2
// Author: GlennCB (glenncb@p3pd.com)
// Created: Wednesday, February 14, 2024 6:55:14 AM
// Copyright: (C) 2024, Phase Three Product Development
//-----------------------------------------------------

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/rmt.h>
#include <Arduino.h>
#include "boards.h"
#include "globals.h"
#include "pins.h"
#include "led.h"
#include "modbus.h"
#include "adc.h"
#include "dac.h"
#include "init.h"


// flass an error code on LEDs every <n> loop iterations if we're in the FAULT state
#define FAULTLPCNT_SENDCODE 10000

// board and firmware revision information (do not modify)
const boardinfo_t boardinfo = {.boardrev=static_cast<uint8_t>(boardtab[BOARD_ID].rev-'A'), .boardid=BOARD_ID};
const fwinfo_t fwinfo = {.fwrev_minor=FWREV_MINOR, .fwrev_major=FWREV_MAJOR};

typedef struct { ledcolor_e color1, color2; } fsmcolors_t;

const fsmcolors_t FsmColorTable[] = {
    {ledGREEN,  ledGREEN},  // RESET
    {ledGREEN,  ledGREEN},  // IDLE
    {ledGREEN,  ledRDGRN},  // HOMING
    {ledGREEN,  ledBLACK},  // INDEXING
    {ledRED,    ledBLACK}   // FAULT
};

// globals
//stepstatus_e stepstatus = stepIDLE;
faults_e faulterr = faultNONE;

fsmstates_e fsm_state;
fsmstates_e fsm_nextstate, fsm_laststate;
boardctl_t boardctl;
uint8_t clrwdt_en = 0;
unsigned char fsmcolor = 0;
unsigned char prevfsmcolor = 0;
uint16_t faultlpcnt = 0;

boardstat_t boardstat;

float setpress, monpress, omegapress;
uint16_t loadcell;

// serial number and modbus-accessible control/PID parameters
uint16_t serialno = 0;
uint16_t ctlmode = 0;
float temp_target = 0.0;
uint16_t fan_interval = 0;
float fan_kp = 0.0, fan_ki = 0.0, fan_kd = 0.0;
int16_t wifirssi = 0;

// regulator serial baud rate
uint16_t regbaud = REG_BAUDRATE_DEFAULT;

// timer module for LED flashing
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void adcMeasure(void);
void dacWrite(void);
//uint16_t cnt = 0;

//---------------------
// setup() - initialization entry point for Arduino
//---------------------
void setup(void)
{
    // clear boardctl and stat, these are updated during init
    boardctl.val = 0;
    boardstat.val = 0;
    clrwdt_en = 0;

    // setup the serial console
    Serial.begin(115200);
    delay(2000); // wait for time for console to connect


    // print the revision information
    Serial.println("FluidManifold (c) 2026 Phase Three Product Development, Inc.");
    Serial.println("=============================================================");
    Serial.printf("Board %s, PCB Revision %c, Firmware Version %s\n", boardtab[BOARD_ID].name, boardtab[BOARD_ID].rev, VERSION);
    Serial.println("=======================================================");
    delay(1000);

    // call initSystem to initialize the various systems and keep the main routine clean
    initSystem();

    // initialize the FSM
    fsm_state = stRESET;
    fsm_nextstate = stRESET;
    fsm_laststate = stRESET;

    SetFsmColorIndex(0);
    prevfsmcolor = 1;
    faultlpcnt = 0;
}
  
  /********************/
  /* main loop FSM    */
  /********************/
  void loop()
  {
    ledcolor_e color;
    unsigned char fault;
    fsmcolors_t colorpair;
    // clear the watchdog timer if the interrupts are all working ok
    // on each timer3 interrupt, set the clrwdt_en
    // this way, if either main loop or interrupts stop working, we'll reset
    if(clrwdt_en)
    {
        //CLRWDT(); // FIXME - decide if we need WDT 
        clrwdt_en = 0;
    }
    
    // FSM
    fsm_nextstate = fsm_state; // default that we stay in current state
    
    // update the error status
    boardstat.bits.error = (faulterr != faultNONE);
    //boardstat.bits.busy = (stepstatus != stepIDLE);

    // handle board control requests

    //------------------------------------
    // board reset
    //------------------------------------
    if(boardctl.bits.reset == 1)
    {
        // reset the processor
        Serial.println("Reset requested - restarting...");
        delay(1000);
        ESP.restart();
    }

    //------------------------------------
    // clear error condition
    //------------------------------------ 
    if(boardctl.bits.clrerror == 1)
    {
        // reset the state machine
        fsm_nextstate = stRESET;
        // clear the fault
        faulterr = faultNONE;
        
        // now clear the clrerr and error bits
        boardctl.bits.clrerror = 0;
    }

    switch (fsm_state)
    {
        case stRESET: // initial state
            fsm_nextstate = stIDLE; // move immediately to the homing state
            break;
        case stIDLE: // IDLE
            // while in the idle state, look for any incoming commands
            break;
        case stFAULT:
            fsm_nextstate = stFAULT;
            // check if the user is attempting to clear the fault by pulling down the FAULT pin
            if(boardctl.bits.clrerror) // user is attempting to clear the fault
            {
                // go to the idle state, but user should probably HOME after a fault
                fsm_nextstate = stIDLE;
                faulterr = faultNONE;
            }
            break;
        default: // we shouldn't reach here
            faulterr = faultFSM;
            fsm_nextstate = stFAULT;  
    }
    
    // check if there were any errors, if so, set fsm_nextstate to stFAULT and assert FAULT pin
    if(faulterr != faultNONE) 
    { 
        // set fsm state to FAULT state
        fsm_nextstate = stFAULT;

        if(++faultlpcnt > FAULTLPCNT_SENDCODE)
        {
            // flash out the error code number on the LED
            LEDSendErrorCode(faulterr);
            faultlpcnt = 0; // reset loop counter             
        }
    }
  
    //------------------------------------
    // set the correct color to send to LEDs based on the state, generate blinking lights
    //------------------------------------
    if((fsmcolor != prevfsmcolor) || (fsm_state != fsm_laststate))
    {
        prevfsmcolor = fsmcolor;
        colorpair = FsmColorTable[fsm_state];
        color = fsmcolor ? colorpair.color1 : colorpair.color2;
        // set current color
        LEDSetColor(color);
        // send the serial RGB color data to the LEDs
        LEDSend(); 
        // delay long enough for the data to reach LED
        delay(10);
    }
    
    // advance the FSM to the next state
    fsm_laststate = fsm_state;
    fsm_state = fsm_nextstate;

    //adcMeasure();
    //dacWrite();
    
    // kill some time
    delay(10);

}

// choose between primary color (0) or secondary color (1)
// the timer ISR will switch between primary/secondary every .25 sec
// the main loop will then send this color to the LED
void SetFsmColorIndex(unsigned char colornum)
{
    fsmcolor = colornum & 1;
}

// set the current fault status to <fault>
void SetFault(faults_e fault)
{
    // save the fault type
    // FIXME - should faults be enums or bits that are ORed together?
    faulterr = fault;
}

// TMR0 interrupt handler, triggers every 250ms
// toggle between primary color (0) or secondary color (1), switch each interrupt
// the main loop will then send this color to the LED
void IRAM_ATTR TimerChangeColorInterruptHandler(void)
{
    // enable clearing the WDT as interrupts are working
    clrwdt_en = 1;
    
    // switch the active color
    fsmcolor ^= 1;
    
}

// print debug message to serial console for access from C files
void serlog(const char *msg)
{
  Serial.println(msg);
}

void adcMeasure()
{
    monpress   = adc[0].Read(0) * REG_ADC_CORRECTION;
    omegapress = adc[0].Read(1);
    loadcell   = (uint16_t) adc[0].ReadRaw(2);
}

static uint16_t lastval = 0;
void dacWrite()
{
    uint32_t lval;
    uint16_t val;

    lval = (uint32_t)((double)(setpress / REG_MAXPRESS) * 4095.0 * REG_DAC_CORRECTION);
    val  = (lval > 0x0FFF) ? 0x0FFF : (uint16_t)(lval & 0x0FFF);

    if (val != lastval)
        lastval = val;

    dac.Write(val);
}

