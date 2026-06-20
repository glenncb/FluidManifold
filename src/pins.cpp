#include <iostream>
#include <Arduino.h>
#include "cppgpio/cppgpio.h"
#include "pins.h"

// GPIO Pin Map for Fluid Manifold PCB Rev B (087-09002B)
/*
| GPIO Pin | Signal Name       | I/O, active state | Description                                     |
| :------: | :---------------: | :---------------: | :---------------------------------------------- |
| IO0      | BOOT              | I, level          | Boot mode select (0=serial, 1=normal)           |
| IO1      | SCL               | IO, low           | I2C clock                                       |
| IO2      | SDA               | IO, low           | I2C data                                        |
| IO3      |                   |                   | NC                                              |
| IO4      |                   |                   | NC                                              |
| IO5      |                   |                   | NC                                              |
| IO6      | LEDSI             | O                 | serial status LED (WS2812B LED1 DIN)            |
| IO7      | ERRLED            | O, low            | red error LED (D2, active low)                  |
| IO8      |                   |                   | NC                                              |
| IO9      | VACUUM_SWITCH     | I, high           | fixed setpoint vacuum switch (J13)              |
| IO10     | FAN_ALERT         | I, low            | EMC2302 fan controller alert (U15 ALERT)        |
| IO11     | BOOST_REGULATOR   | O                 | boost pressure regulator solenoid (U5 1B, J10)  |
| IO12     | SAMPLE_REGULATOR  | O                 | sample pressure regulator solenoid (U5 2B, J11) |
| IO13     | PINCH_FULL        | O                 | pinch valve full power (U5 3B, J12)             |
| IO14     | PINCH_SOFT        | O                 | pinch valve soft mode (U5 6B+7B, J12)           |
| IO15     |                   |                   | NC                                              |
| IO16     | TXREG             | O, high           | Alicat RS-485 UART1 TX (U10 D)                  |
| IO17     | RXREG             | I, high           | Alicat RS-485 UART1 RX (U10 R)                  |
| IO18     | DIRREG            | O                 | Alicat RS-485 direction, high=TX (U10 DE/RE)    |
| IO19     | USBD-             | I                 | ESP32 USB D- serial console / debug             |
| IO20     | USBD+             | I                 | ESP32 USB D+ serial console / debug             |
| IO21     | TX485             | O, high           | UART2 TxD to main RS-485 (U2 D)                |
| IO26     |                   |                   | NC (do not use)                                 |
| IO33     | RX485             | I, high           | UART2 RxD from main RS-485 (U2 R)              |
| IO34     | DIR485            | O, high           | UART2 direction for RS-485, high=TX (U2 DE/RE) |
| IO35     | NOZZLE_1          | O                 | nozzle 1 solenoid valve (U4 1B, J3)             |
| IO36     | NOZZLE_SELECT_1   | O                 | nozzle select 1 solenoid valve (U4 2B, J4)      |
| IO37     | NOZZLE_2          | O                 | nozzle 2 solenoid valve (U4 3B, J5)             |
| IO38     | NOZZLE_SELECT_2   | O                 | nozzle select 2 solenoid valve (U4 4B, J6)      |
| IO39     | WASTE_CATCHER     | O                 | waste catcher solenoid valve (U4 5B, J7)        |
| IO40     | DRIP_TRAY         | O                 | drip tray solenoid valve (U4 6B, J8)            |
| IO41     | SAMPLE_STATION    | O                 | sample station illumination LED (U4 7B, J9)     |
| IO42     |                   |                   | NC                                              |
| IO45     |                   |                   | NC                                              |
| IO46     |                   |                   | NC                                              |
| IO47     |                   |                   | NC                                              |
| IO48     |                   |                   | NC                                              |
*/

// create globals for the GPIOs so they can be used across the different files
// common GPIOs to all boards
CPPGPIO::Gpio pinSTATLED                {GPIO_MODE_OUTPUT, GPIO_NUM_6, GPIO_ACTIVE_LOW};
CPPGPIO::Gpio pinERRLED                 {GPIO_MODE_OUTPUT, GPIO_NUM_7, GPIO_ACTIVE_LOW};
CPPGPIO::Gpio pinBOOTSW                 {GPIO_MODE_INPUT,  GPIO_NUM_0};
// Board specific GPIOs
CPPGPIO::Gpio pinVACUUM_SWITCH          {GPIO_MODE_INPUT,  GPIO_NUM_9, GPIO_ACTIVE_LOW};
CPPGPIO::Gpio pinFAN_ALERT              {GPIO_MODE_INPUT,  GPIO_NUM_10, GPIO_ACTIVE_LOW};
CPPGPIO::Gpio pinBOOST_REGULATOR        {GPIO_MODE_OUTPUT, GPIO_NUM_11};
CPPGPIO::Gpio pinSAMPLE_REGULATOR       {GPIO_MODE_OUTPUT, GPIO_NUM_12};
CPPGPIO::Gpio pinPINCH_FULL             {GPIO_MODE_OUTPUT, GPIO_NUM_13};
CPPGPIO::Gpio pinPINCH_SOFT             {GPIO_MODE_OUTPUT, GPIO_NUM_14};
CPPGPIO::Gpio pinNOZZLE_1               {GPIO_MODE_OUTPUT, GPIO_NUM_35};
CPPGPIO::Gpio pinNOZZLE_SELECT_1        {GPIO_MODE_OUTPUT, GPIO_NUM_36};
CPPGPIO::Gpio pinNOZZLE_2               {GPIO_MODE_OUTPUT, GPIO_NUM_37};
CPPGPIO::Gpio pinNOZZLE_SELECT_2        {GPIO_MODE_OUTPUT, GPIO_NUM_38};
CPPGPIO::Gpio pinWASTE_CATCHER          {GPIO_MODE_OUTPUT, GPIO_NUM_39};
CPPGPIO::Gpio pinDRIP_TRAY              {GPIO_MODE_OUTPUT, GPIO_NUM_40};
CPPGPIO::Gpio pinSAMPLE_STATION         {GPIO_MODE_OUTPUT, GPIO_NUM_41};

// GPIO array for modbus driver access — initialized by modGpioInit() in modbusmap_gen.cpp
CPPGPIO::Gpio pinGPIO[NUM_GPIO_PINS];

void  gpio_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    Serial.printf("GPIO event handler processed interrupt from pin ID: %d\n", id);
}

// routine to setup the GPIOs using the cppgpio library
void setupGPIOs(void) {
    // GPIOs are initialized statically by the constructor in the pins.h file

    // Setup an interrupt handler for the BOOTSW pin

    // first we need to create the default event handler loop
    esp_event_loop_create_default();

    // set the pin to trigger an interrupt on the falling edge
    pinBOOTSW.enableInterrupt(GPIO_INTR_NEGEDGE);
    // register the event handler for the pin
    pinBOOTSW.setEventHandler(&gpio_event_handler);
}
