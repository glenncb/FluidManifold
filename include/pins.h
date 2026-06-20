#pragma once

#include "cppgpio/cppgpio.h"
#include "esp_log.h"

#define GPIO_ACTIVE_LOW 1

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
| IO9      | VACUUM_SWITCH     | I, low            | fixed setpoint vacuum switch (J13)              |
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

// Alicat regulator UART1 pins
#define PIN_RXREG  GPIO_NUM_17
#define PIN_TXREG  GPIO_NUM_16
#define PIN_DIRREG GPIO_NUM_18

// Main RS-485 modbus UART2 pins
#define PIN_RX485   GPIO_NUM_33
#define PIN_TX485   GPIO_NUM_21
#define PIN_DIR485  GPIO_NUM_34

// I2C pins (defaults for Wire.begin() in Arduino)
#define PIN_SDA GPIO_NUM_2
#define PIN_SCL GPIO_NUM_1

// Serial LED pin (WS2812B via RMT module)
#define PIN_LEDSI       GPIO_NUM_6

// declare pin assignments to setup I/Os
// standard pins across all boards
extern CPPGPIO::Gpio pinSTATLED;
extern CPPGPIO::Gpio pinERRLED;
extern CPPGPIO::Gpio pinBOOTSW;
// board specific pins
extern CPPGPIO::Gpio pinVACUUM_SWITCH;
extern CPPGPIO::Gpio pinFAN_ALERT;
extern CPPGPIO::Gpio pinBOOST_REGULATOR;
extern CPPGPIO::Gpio pinSAMPLE_REGULATOR;
extern CPPGPIO::Gpio pinPINCH_FULL;
extern CPPGPIO::Gpio pinPINCH_SOFT;
extern CPPGPIO::Gpio pinNOZZLE_1;
extern CPPGPIO::Gpio pinNOZZLE_SELECT_1;
extern CPPGPIO::Gpio pinNOZZLE_2;
extern CPPGPIO::Gpio pinNOZZLE_SELECT_2;
extern CPPGPIO::Gpio pinWASTE_CATCHER;
extern CPPGPIO::Gpio pinDRIP_TRAY;
extern CPPGPIO::Gpio pinSAMPLE_STATION;

// GPIO array for modbus driver access (indexed by drvchan in coilmap/contmap)
// drvchan 0=BOOTSW, 1=ERRLED, 2=BOOST_REGULATOR, 3=SAMPLE_REGULATOR,
//         4=PINCH_FULL, 5=PINCH_SOFT, 6=NOZZLE_1, 7=NOZZLE_SELECT_1,
//         8=NOZZLE_2, 9=NOZZLE_SELECT_2, 10=WASTE_CATCHER, 11=DRIP_TRAY,
//         12=SAMPLE_STATION, 13=VACUUM_SWITCH, 14=FAN_ALERT
#define NUM_GPIO_PINS 15
extern CPPGPIO::Gpio pinGPIO[NUM_GPIO_PINS];

// initialize GPIOs
void setupGPIOs(void);
