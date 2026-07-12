/* 
 * File:   globals.h
 * Author: Glenn Colon-Bonet
 * Comments: Globals / types / defines used across routines from main.cpp
 * Revision history: 
 * - 2024-02-18: updates for ESP32 C++ env
 */
#pragma once
// This is a guard condition so that contents of this file are not included
// more than once.  

#include <stdint.h>
#include <Arduino.h>
#include "boards.h"

// set the board type and current firmware revision
#define BOARD_ID MANIFOLD
// Modbus address for this device (set to same as board ID for now)
#define MODBUS_ADDR BOARD_ID

#define REVISION 0.17
/* Revision history
 * 0.18 - 07/12/2026  - bump rev number purely for testing
 * 0.17 - 07/12/2026  - performing a bulk erase instead of block by block to improve reliability and performance
 * 0.16 - 07/11/2026  - added retry on the host side, fixed state machine bug at end of update, added patch to get the version info in bin file build header
 * 0.15 - 07/11/2026  - initial somewhat working version of OTA firmware updates
 * 0.14 - 07/11/2026  - bumped rev to test OTA firmware update over modbus, added strings for board and rev tags, speedups to ota
 * 0.13 - 07/07/2026  - OTA firmware update over Modbus (FWUPDSTART/END/ABORT coils, FWSIZE/FWCHECKSUM/FWDATA holdregs, FWUPDSTATE/ERR/PERCENT inregs); switched to OTA partition board
 * 0.12 - 06/22/2026  - calibrated adc/dac values
 * 0.11 - 06/20/2026  - removed SHE_BOOST/SHE_ATM (NovaCart artifacts), fixed VACUUM_SWITCH active-low polarity, corrected Alicat pressure range to ±5 PSI differential, updated SAMPLE_STATION description to illumination LED
 * 0.10 - 06/19/2026  - initial version ported from NovaCart, updated GPIO pin assignments for Rev B PCB
 */

// convert the revision number to a string
#define STRINGIZE(X) _STRINGIZE(X)
#define _STRINGIZE(X) #X
#define VERSION STRINGIZE(REVISION)

// macros to extract major and minor revision from REVISION
#define FWREV_MAJOR ((int)(REVISION))
#define FWREV_MINOR ((int)((REVISION - FWREV_MAJOR) * 100))

#define DAC_ADDRESS 0x60

typedef enum __attribute__((packed)) { faultNONE, faultCOMM, faultADC, faultDAC, faultCPUNDERVOLT, faultFSM } faults_e;
//typedef enum __attribute__((packed)) { stepIDLE, stepACTIVE, stepDONE } stepstatus_e;

// additional outputs
typedef union {
	uint8_t val;
	struct __attribute__((packed)) {
        unsigned int clrerror   	: 1;    // writing to a 1 will clear any error condition
		unsigned int reset			: 1;    // performs a processor reset when written to a 1
		unsigned int debug   		: 1;    // debug mode - enables some features and also enables verbose serial console error messages
		unsigned int regmode_serial : 1;	// Selects serial regulator control/monitor when set, else analog
		unsigned int reserved   	: 4;    // reserved for future use
	} bits;
} boardctl_t;

typedef union {
	uint16_t val;
	struct __attribute__((packed)) {
		unsigned int busy      		: 1;	// unit is busy actively moving
		unsigned int error     		: 1;	// error has occurred, read fault code for more information
		unsigned int serregerr 		: 1;	// set if regulator in serial mode and comm error occurs
		unsigned int i2cadcerr 		: 1;	// set if I2C ADC not present at proper I2C address or has comm error
		unsigned int i2cdacerr 		: 1;	// set if I2C DAC not present at proper I2C address or has comm error
		unsigned int i2cfan    		: 1;	// set if I2C fan controller not present or has comm error
		unsigned int fanstall  		: 2;	// bit per fan: set if fan has stalled (bit0=fan0, bit1=fan1)
		unsigned int i2ctemp   		: 2;	// bit per temp sensor: set if I2C temp sensor not present (bit0=sensor0, bit1=sensor1)
		unsigned int reserved  		: 6;
	} bits;
} boardstat_t;

extern const boardinfo_t boardinfo;
extern const fwinfo_t fwinfo;

extern boardctl_t boardctl;
extern boardstat_t boardstat;
extern faults_e faulterr;

extern float setpress, monpress, omegapress;
extern uint16_t loadcell;

// serial number cached from flash
extern uint16_t serialno;

// control/PID parameters exposed over modbus
extern uint16_t ctlmode;
extern float temp_target;
extern uint16_t fan_interval;
extern float fan_kp, fan_ki, fan_kd;
extern int16_t wifirssi;

// regulator serial baud rate
extern uint16_t regbaud;

extern uint8_t clrwdt_en;

typedef enum { stRESET, stIDLE, stHOMING, stINDEXING, stNUDGING, stFAULT } fsmstates_e;
extern fsmstates_e fsm_state, fsm_nextstate, fsm_laststate;
extern unsigned char fsmcolor, prevfsmcolor;
extern uint16_t faultlpcnt;

// regulator set/monitor full scale pressure value in PSI (differential, ±5 PSI)
#define REG_MAXPRESS  5.0
#define REG_MINPRESS -5.0
#define OMEGA_MAXPRESS 150.0
// correction factor to compensate for resistive divider on pressure monitor (10K/6.65K = 3.0V full scale, vs. 5.0V ADC Vref full scale)
#define REG_ADC_CORRECTION (1.6804/1.12)

// DAC correction factor
#define REG_DAC_CORRECTION 1.109

#define REG_BAUDRATE_DEFAULT 19200

#ifdef __cplusplus
extern "C" {
#endif
 	// functions
	void SetFsmColorIndex(unsigned char colornum);
	void SetFault(faults_e fault);
	// set the current fault status to <fault>
    //inline void SetStepStatus(stepstatus_e stepstat) { stepstatus = stepstat; }
	// set the current fault status to <fault>
	//inline stepstatus_e GetStepStatus(void) { return stepstatus; }

	void serlog(const char *msg);

	// LED blink timer3 handler
	void TimerChangeColorInterruptHandler(void);
#ifdef __cplusplus
}
#endif
