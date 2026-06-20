// Alicat PCD series pressure regulator
// RS-485 UART1 communication using Alicat serial protocol

#include <Arduino.h>
#include <stdint.h>
#include "alireg.h"
#include "globals.h"
#include "pins.h"
#include "modbusmap_gen.h"

ALIREG::ALIREG(void)
{
    addr = 0xFF;
    aregtype = AREG_OFFLINE;
}

ALIREG::~ALIREG(void)
{
}

void ALIREG::Init(uint8_t ad)
{
    int retries=0;
    addr = ad;
    ESP_LOGI("ALIREG::Init", "Initializing ALIREG: address=%0x (%c)", ad, AddrCh());

    if (alireg485InUse == NULL)
    {
        alireg485InUse = xSemaphoreCreateBinary();
        xSemaphoreGive(alireg485InUse);
    }

    if (!ALIREG_UART)
    {
        ESP_LOGE("ALIREG::Init", "ALIREG_UART is not open.");
        return;
    }

    if(ALIREG_UART.available())
        ALIREG_UART.readString(); // flush any pending data

    // try reading from the device a few times to establish communication before giving up and marking the device as offline
    for(retries=0; retries<3; retries++) 
    {
        if (ReadDeviceData())
        {
            break;
        }
    }

    if (retries >= 3)
    {
        ESP_LOGE("ALIREG::Init", "Unable to establish communication with Alicat device at addr %d.", ad);
        aregtype = AREG_OFFLINE;
        return;
    }

    aregtype = AREG_ONLINE;

    if(!ReadDeviceInfo())
    {
        ESP_LOGE("ALIREG::Init", "Unable to read device info from Alicat device at addr %d.", ad);
    }
}

bool ALIREG::ReadDeviceInfo(void)
{
    String sbuf;
    char ad;
    int n;

    ESP_LOGI("ALIREG::ReadDeviceInfo", "Reading device info from Alicat device %c.", AddrCh());

    // get the manufacturing information string from the device
    ALIREG_UART.printf("%c??M*\r", AddrCh());
    delay(100);
    sbuf = ALIREG_UART.readStringUntil('\r');

    if (sbuf.length() == 0)
    {
        ESP_LOGE("ALIREG::ReadDeviceInfo", "No response from Alicat device %c.", AddrCh());
        return false;
    }

    Serial.printf("Alicat device %c info: \n\t%s\n", AddrCh(), sbuf.c_str());
    while(ALIREG_UART.available())
    {
        sbuf = ALIREG_UART.readStringUntil('\r');
        if (sbuf.length() > 0)
        {
            // echo the info lines to the console log
            Serial.printf("\t%s\n", sbuf.c_str());
        }
        else
        {
            break;
        }
    }
    return true;
}

// Poll the regulator: send address char, parse "A <press> <setpoint>"
bool ALIREG::ReadDeviceData(void)
{
    String sbuf;
    char ad;
    int n;

    ESP_LOGI("ALIREG::ReadDeviceData", "Reading data from Alicat device %c.", AddrCh());

    // clear any stray data in the buffer before sending the command, otherwise we might read old data instead of the response to our command
    if(ALIREG_UART.available())
        ALIREG_UART.readString(); // flush any pending data

    ALIREG_UART.printf("%c\r", AddrCh());
    ALIREG_UART.flush(); // ensure the command is sent before we wait for the response
    //delay(100);
    sbuf = ALIREG_UART.readStringUntil('\r');

    if (sbuf.length() == 0)
    {
        ESP_LOGE("ALIREG::ReadDeviceData", "No response from Alicat device %c.", AddrCh());
        return false;
    }

    n = sscanf(sbuf.c_str(), "%c %f %f", &ad, &aregdata.press, &aregdata.setpoint);
    if (n != 3)
    {
        ESP_LOGE("ALIREG::ReadDeviceData", "Error parsing response from Alicat device %c: resp=\"%s\", matched=%d", AddrCh(), sbuf.c_str(), n);
        return false;
    }

    aregdata.lastread = millis();
    ESP_LOGI("ALIREG::ReadDeviceData", "Read from Alicat device %c: press=%.4f, setpoint=%.4f", AddrCh(), aregdata.press, aregdata.setpoint);
    return true;
}

// Set the pressure setpoint: command is "<addr>s<value>\r"
void ALIREG::SetPressure(float sp)
{
    ALIREG_UART.printf("%cS%.4f\r", AddrCh(), sp);
    ALIREG_UART.readStringUntil('\r'); // consume the echo/ack
    ESP_LOGE("ALIREG::SetPressure", "Set pressure setpoint on device %c to %.4f", AddrCh(), sp);
}

void modr_ALIREG(uint16_t *val, uint32_t drvaddr, uint8_t drvchan)
{
    float fv = -666.0;

    if (!val)
    {
        ESP_LOGE("modr_ALIREG", "ERROR: val is NULL.");
        return;
    }

    if (drvaddr >= ALIREG_NUM)
    {
        ESP_LOGE("modr_ALIREG", "ERROR: drvaddr %d is out of range.", drvaddr);
        return;
    }

    if (!areg[drvaddr].Online())
    {
        ESP_LOGE("modr_ALIREG", "ERROR: ALIREG device at address %d is offline.", drvaddr);
        memcpy(val, &fv, sizeof(float));
        return;
    }

    xSemaphoreTake(alireg485InUse, portMAX_DELAY);
    switch (drvchan)
    {
        case ALIREG_MONPRESS:
            fv = areg[drvaddr].GetPress();
            break;
        case ALIREG_SETPRESS:
            fv = areg[drvaddr].GetSetpoint();
            break;
        default:
            ESP_LOGE("modr_ALIREG", "ERROR: drvchan %d is not valid.", drvchan);
            break;
    }
    xSemaphoreGive(alireg485InUse);

#if MODBUS_DEBUG
    Serial.printf("modr_ALIREG: channel %d = %f\n", drvchan, fv);
#endif
    memcpy(val, &fv, sizeof(float));
}

void modw_ALIREG(uint16_t *val, uint32_t drvaddr, uint8_t drvchan)
{
    if (!val)
    {
        ESP_LOGE("modw_ALIREG", "ERROR: val is NULL.");
        return;
    }

    if (drvaddr >= ALIREG_NUM)
    {
        ESP_LOGE("modw_ALIREG", "ERROR: drvaddr %d is out of range.", drvaddr);
        return;
    }

    if (!areg[drvaddr].Online())
    {
        ESP_LOGE("modw_ALIREG", "ERROR: ALIREG device at address %d is offline.", drvaddr);
        return;
    }

    float fv = *((float *) val);

    xSemaphoreTake(alireg485InUse, portMAX_DELAY);
    switch (drvchan)
    {
        case ALIREG_SETPRESS:
            areg[drvaddr].SetPressure(fv);
            break;
        default:
            ESP_LOGE("modw_ALIREG", "ERROR: write to drvchan %d is not valid.", drvchan);
            break;
    }
    xSemaphoreGive(alireg485InUse);

#if MODBUS_DEBUG
    Serial.printf("modw_ALIREG: channel %d = %f\n", drvchan, fv);
#endif
}

void initAliReg(void)
{
    ESP_LOGI("initAliReg", "Initializing ALIREG UART baud rate to %d.", ALIREG_BAUD);
    // initialize the RS485 Alicat regulator serial port
    ALIREG_UART.begin(ALIREG_BAUD, SERIAL_8N1, PIN_RXREG, PIN_TXREG);
    // Enable hardware serial direction control on the DIR pin
    if(!ALIREG_UART.setPins(-1, -1, -1, PIN_DIRREG))
    {
        ESP_LOGE("initAliReg", "Error: setPins: unable to set the Alicat UART DIR pin.");
    }

    // set RS485 half duplex mode (automatically control the DIR pin)
    if(!ALIREG_UART.setMode(UART_MODE_RS485_HALF_DUPLEX))
    {
        ESP_LOGE("initAliReg", "Error: setMode: unable to set the Alicat UART to half duplex mode.");
    }

    // initialize the ALIREG objects for each address used (generated from modbusmap.xlsx into modbusmap_gen.cpp)
    modAliRegInit();

    // Set the pressure to 40 PSI for testing
    //areg[0].SetPressure(40.0);
    // read back the pressure to test
    //float testPressure = areg[0].GetSetpoint();
    //ESP_LOGE("initAliReg", "Test readback from Alicat device %c: press=%.4f", areg[0].AddrCh(), testPressure);
}

ALIREG areg[ALIREG_NUM];
SemaphoreHandle_t alireg485InUse = NULL;
