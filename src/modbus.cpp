// modbus.cpp - modbus RTL server (slave) implementation

//----------------------------------------------------
// file: modbus.cpp
// description: Modbus server implementation (modbus RTU slave)
// language: C++
// Target processor: ESP32-S3-MINI-N4R2
// Author: GlennCB (glenncb@p3pd.com)
// Created: Wednesday, February 14, 2024 6:55:14 AM
// Copyright: (C) 2024, Phase Three Product Development
//-----------------------------------------------------
#include <Arduino.h>
// pin definitions
//#include "pins.h"
// definitions for modbus server
#include "modbus.h"
#include "modbusmap.h"
#include "modbusota.h"
#include "globals.h"
#include "pins.h"

// General modbus implementation algorithm
// Uses tasks and event queues to keep the modbus server code as lean as possible
// - define a table mapping the virtual modbus resources to the actual resources used in the code
//       > also defines which resources are valid for the function code(s) and whether readonly or read/write
// - initialize modbus server
//   Defines all of the function code callbacks for each of the supported modbus function codes
//   Setup the event loop queue for triggering events on writes to coils and holding registers
//       > use events rather than direct processing to reduce overhead
//   Register the event callbacks for the event loop queue
//   Start the event loop handling task to run in the background
// - event loop task
//   Lower priority task to check for events in the event loop queue and process them
//      > copies data from coils/holding regs to actual resources or invokes a function to handle the event(s)

// Create Modbux Server object with 1 second timeout
#if USE_MODBUS_RTU
ModbusRTU MB;
#else
ModbusIP MB;
#endif
// semaphore for local / remote modbus access
SemaphoreHandle_t modbusInUse;

// modbusServer - modbus server task - repeatedly calls the ModbusRTU task() function
void modbusServer(void *)
{
  long int cnt=0;
  while(1) {
    // check for modbus events
    cnt++;
    MB.task();
    if(cnt%1000 == 0) {
      Serial.print(".");
    } 
    vTaskDelay(MODBUS_SERVER_DELAY);
  }
}

// modbusStartServer - start the modbus server task 
void modbusStartServer() 
{
  // start the modbus server task in the background on core 0 (avoid Arduino loop() on core 1 for better performance on Modbus)
  //xTaskCreatePinnedToCore(modbusServer, "modbusServer", 4096, NULL, 3, NULL, 0);
  // start the modbus server task in the background - for debug, easier to debug if on same core as Arduino loop
  xTaskCreate(modbusServer, "modbusServer", 4096, NULL, 5, NULL);
}

//----------------------------------------------------
// modbusSetup - setup and start the modbus server
//----------------------------------------------------
void modbusSetup()
{
  #if USE_MODBUS_RTU
  // Init Serial2 connected to the RTU Modbus
  MODBUS_UART.begin(MODBUS_BAUD, SERIAL_8N1, PIN_RX485, PIN_TX485);
  // setup the modbus server on the targeted serial port with direction control 
  // using DIR485 pin, positive true (transmits when set high)
  MB.begin(&MODBUS_UART, PIN_DIR485, true);
  // set the modbus baudrate
  MB.setBaudrate(MODBUS_BAUD);
  // set the modbus slave address
  MB.slave(MODBUS_SLAVE_ADDR);
  Serial.printf("Modbus RTU server started on Serial2 at %d baud, slave address %d\n", MODBUS_BAUD, MODBUS_SLAVE_ADDR);
#else
  // create the ModbusIP server
  MB.server();
#endif
  // create the modbus semaphore
  modbusInUse = xSemaphoreCreateBinary();
  // release the semaphore
  xSemaphoreGive(modbusInUse);
  // create the coil, contact, input register, and holding registers
  modbusRegSetup(&MB);
  // initialize OTA firmware update over modbus (after the registers exist)
  modbusOTASetup();
  modbusStartServer();
}
