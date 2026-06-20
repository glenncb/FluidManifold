/****************************************************************/
/* led.h - routines to set/flash the status and error LEDs		*/
/*--------------------------------------------------------------*/
/* External Entry Points:					                    */
/* LEDInit() - init pin and set all LEDs to black		        */
/* LEDSetColor(l,c) - set led<l> to color <c>			        */
/* LEDSendData() - update the LEDs			                    */
/*--------------------------------------------------------------*/
/* Modified: Sun Nov 30 12:17:25 MST 2014 (gcb)			        */
/* - 2024-02-18: updated for C++ ESP32 build env                */
/****************************************************************/
#pragma once

#include "globals.h"

// maximum number of serial LEDs allowed
#define MAXLEDS 8

// 5050 versions of the serial LED are GRB ordered but 5mm version is RGB order
//#define GRB_ORDER 1

// predefined color palette for use with the serial LEDs
typedef enum __attribute__((packed)) { ledBLACK, ledRED, ledGREEN, ledRDGRN } ledcolor_e;

#ifdef __cplusplus
extern "C" {
#endif
// initialize port pin and init all LEDs to black
void LEDInit(void);

// set LED <lednum> to <color>
void LEDSetColor(ledcolor_e color);

// Send serial data to the NeoPixels in GRB serial format
void LEDSend(void);

void LEDSendErrorCode(faults_e fault);
#ifdef __cplusplus
}
#endif