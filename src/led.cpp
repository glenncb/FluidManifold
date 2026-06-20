/****************************************************************/
/* serled.c - routines for managing WS2812/WS2812b serial LEDs	*/
/* Contains defs and routines used locally for serial LEDs.	*/
/* External code should only reference serled.h for protos of	*/
/* top level entry points and external data type defs		*/
/*--------------------------------------------------------------*/
/* External Entry Points:					*/
/* LEDInit() - init pin and set all LEDs to black		*/
/* LEDSetColor(l,c) - set led<l> to color <c>			*/
/* LEDSendData() - send serial data to LEDs			*/
/*--------------------------------------------------------------*/
/* Modified: Sun Nov 30 11:20:25 MST 2014 (gcb)			*/
/****************************************************************/
#include <stdint.h>
#include "pins.h"
#include "led.h"
#include "globals.h"

// serial LED RGB value
typedef struct { unsigned char r;  unsigned char g; } ledrg_t;

// led color struct
ledrg_t led;

// pre-initialized RGB color table to map the enumerated ledcolor_e
// values to the corresponding RGB values
const ledrg_t ColorTable[] = {
    // r,   gl
    {0,     0},     // black
    {1,     0},     // red
    {0,     1},     // green
    {1,     1}      // redgreen
};

/*------------------------------------------------------------------------------*/
/* LEDSend() - Send serial data to the NeoPixels in GRB serial format		*/
/*------------------------------------------------------------------------------*/
void LEDSend(void)
{
    pinERRLED = led.r;
}



/*------------------------------------------------------------------------------*/
/* LEDSetColor(l,c) - performs a color lookup of <c> to get RGB data and loads	*/
/* into struct for LED index <l>.  Note this only loads the LED pixel array - 	*/
/* the actual data is sent using LEDSend()					*/
/*------------------------------------------------------------------------------*/
void LEDSetColor(ledcolor_e color)
{
    led = ColorTable[color];
}

// toggle LEDs at startup to show processor is alive
void LEDToggle(void)
{
    uint8_t i;
    
    for(i=0;i<2;i++)
    {
        LEDSetColor(ledGREEN);
        LEDSend();
        delay(250);
        LEDSetColor(ledBLACK);
        LEDSend();
        delay(100);
        LEDSetColor(ledRED);
        LEDSend();
        delay(250);
        LEDSetColor(ledBLACK);
        LEDSend();
        delay(100);
    }
}

/*------------------------------------------------------------------------------*/
/* LEDInit() - initialize LED port and set all LEDs to black			*/
/*------------------------------------------------------------------------------*/
void LEDInit(void)
{
    // wiggle the LEDs to show we're alive
    LEDToggle();
    // set the LED color to black
    LEDSetColor(ledBLACK);
    // send the data to the LEDs
    LEDSend();
}

/*------------------------------------------------------------------------------*/
/* flash the error code on the LED                                              */
/*------------------------------------------------------------------------------*/
void LEDSendErrorCode(faults_e fault)
{
    char p;
    
    if(!fault) // there is no error
    {
        return;
    }
    
    // on solid for 1 second
    LEDSetColor(ledRED);
    LEDSend();
    delay(1000);
    // now off for 1 second
    LEDSetColor(ledBLACK);
    LEDSend();
    delay(1000);
    
    // now flash out the error code of the fault
    for(p=0;p<fault;p++)
    {
        // on solid for .5 second
        LEDSetColor(ledRED);
        LEDSend();   
        delay(500);
        // white for .5 second
        LEDSetColor(ledRDGRN);
        LEDSend();   
        delay(500);
    }
    // now off for 1 second
    LEDSetColor(ledBLACK);
    LEDSend();
    delay(1000);
    // on solid for 1 second
    LEDSetColor(ledRED);
    LEDSend();
    delay(1000);
        
    return;
}