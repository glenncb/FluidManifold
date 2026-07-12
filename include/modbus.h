// modbus.h - defines for modbus IP server (slave) implementation
#pragma once
#include "globals.h"
#define USE_MODBUS_RTU 1

#if USE_MODBUS_RTU
#include <ModbusRTU.h>
#define MODBUS_UART Serial2
#define MODBUS_BAUD 460800
//validated works at 2.5Mbaud
//#define MODBUS_BAUD 2500000
#define MODBUS_SLAVE_ADDR MODBUS_ADDR
#else
#include <ModbusIP_ESP8266.h>
#endif

#define MODBUS_SERVER_DELAY 1

// Modbus timeout in ms
#define MODBUS_TIMEOUT 1000

// enable debugging of modbus - WARNING - do not leave this set in production as it slows down response time of modbus server
//#define MODBUS_DEBUG 1

#if USE_MODBUS_RTU
extern ModbusRTU MB; // Create Modbus RTU object with 1 second timeout
#else
extern ModbusIP MB;
#endif

void modbusSetup();             // initialize the modbus server (slave)
void modbusServer(void *);      // modbus server task
//char *modbusGetBoardInfo(void); // return the board and firmware information as a string
// semaphore to prevent simultaneous access to the modbus server from local/remote sources
extern SemaphoreHandle_t modbusInUse;
