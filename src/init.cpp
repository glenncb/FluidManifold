// System initialization routines called from setup()

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>
#include "globals.h"
#include "init.h"
#include "pins.h"
#include "led.h"
#include "serled.h"
#include "modbus.h"
#include "ADS112C04.h"
#include "i2c_acc.h"
#include "adc.h"
#include "dac.h"
#include "emc2302.h"
#include "mcp9808.h"
#include "modbusmap_gen.h"
#include "alireg.h"
#include "eeprom_acc.h"

extern hw_timer_t *timer;

void initSystem(void)
{
    int ret;

    Serial.println("Starting system initialization...");

    Serial.println("Initializing EEPROM...");
    eepromInit(false);
    serialno = readSerial();
    Serial.printf("Board serial number: %d\n", serialno);

    Serial.println("Setting up GPIOs...");
    setupGPIOs();

    Serial.println("Setting up I2C system...");
    I2C_init();

    // init the errLED pin
    LEDInit();

    // initialize the serial LED
    sled.Init(PIN_LEDSI);

    // setup the timer
    timer = timerBegin(100000);
    timerAttachInterrupt(timer, &TimerChangeColorInterruptHandler);
    timerAlarm(timer, 250000, true, 0);

    // flash color lights as sign of life on powerup
    for (int i = 0; i < 2; i++)
    {

        sled.SetColor(sledRED); sled.Send(); delay(250);
        sled.SetColor(sledGREEN); sled.Send(); delay(250);
        sled.SetColor(sledBLUE); sled.Send(); delay(250);
        sled.SetColor(sledYELLOW); sled.Send(); delay(250);
        sled.SetColor(sledWHITE); sled.Send(); delay(250);
    }

    Serial.println("Initialize ADC...");
    initADC();

    Serial.println("Initialize DAC...");
    dac.Init();

    Serial.println("Initialize Temp Sensor...");
    initTemp();

    Serial.println("Initialize the fan controller...");
    initFan();

    Serial.println("Initialize the Alicat pressure regulator...");
    initAliReg();

    //Initilialization routines from modbmusmap_gen
    //GPIO table
    modGpioInit();
    // DAC min/max values
    modDacMinMax();
    // ADC min/max values
    modAdcMinMax();

    Serial.println("Initialize and start Modbus server...");
    modbusSetup();

    Serial.println("Initialization complete.");
}
