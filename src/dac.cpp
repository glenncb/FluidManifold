// DAC 8568 top level functions
#include <Arduino.h>
#include "dac.h"
#include "mcp4725.h"

DAC::DAC(void)
{
    // initialize the min/max val arrays - by default set for a 0-5V range
    maxval = 5.0;
    minval = 0.0;
    val = 0;         // initialize the value to zero
};

DAC::~DAC(void)
{
    // empty for now
};

// initialize the DAC
void DAC::Init(void)
{
    uint32_t msg, resp;
    ESP_LOGI("DAC::Init", "Initializing DAC.");
    // initialize the MCP4725 DAC
    if(!mcp.begin())
    {
        ESP_LOGE("DAC::Init", "Error initializing DAC.");
        return;
    }
};

// write the DAC value to a channel
void DAC::Write(uint16_t data)
{
    ESP_LOGE("DAC::Write", "Writing DAC data %d.", data);
    mcp.setValue(data); // write the value to the DAC
    // set the SPI device to the DAC
    val = data; // save the value
};

// modbus read/write driver functions
// called by the driver for reading PWM parameters
void modr_I2CDAC(uint16_t *val, uint32_t drvaddr, uint8_t drvchan)
{
    float fv;
    uint16_t uv;
    bool rawread = (drvchan < 0);

    drvchan = (drvchan < 0) ? -drvchan : drvchan;

    if(!val)
    {
        ESP_LOGE("modr_DAC", "ERROR: val is NULL.");
        return;
    } 

    // we're not currently using the drvaddr field as there is currently only one DAC
    // use the drvchan field to select which DAC channel to read

    // no semaphore needed as the reads just return a variable and do not use the bus
    if(rawread)
    {
        uv = dac.ReadRaw();
        memcpy(val, &uv, sizeof(uint16_t));
    }
    else
    {
        fv = dac.Read();
        memcpy(val, &fv, sizeof(float));
    }

#if MODBUS_DEBUG
    if(rawread)
        Serial.printf("modr_DAC: channel %d = %04x\n", drvchan, uv);
    else
        Serial.printf("modr_DAC: channel %d = %.2f\n", drvchan, fv);
#endif
}

// called by the driver for writing PWM parameters
void modw_I2CDAC(uint16_t *val, uint32_t drvaddr, uint8_t drvchan)
{
    float fv = *((float *) val);
    bool rawrite = drvchan < 0;
    drvchan = (drvchan < 0) ? (-drvchan) : drvchan; // handle negative drvchan

    ESP_LOGE("modw_DAC", "Writing to DAC channel %d.", drvchan);
    // note this assumes that the modbus registers are stored little endian (low word first)
    if(!val)
    {
        ESP_LOGE("modw_DAC", "ERROR: val is NULL.");
        return;
    }

    // use the drvchan field to select which DAC channel to write, use a negative value to indicate it is a raw write
    if(rawrite)
        dac.Write(*((uint16_t *) val));
    else
        dac.Write(*((float *) val));
    //xSemaphoreGive(spiInUse); // release the SPI bus
#if MODBUS_DEBUG
    Serial.printf("modw_DAC: channel %d = %f\n", drvchan, fv);
#endif
}

// declare the DAC object
DAC dac;

MCP4725 mcp(DAC_I2C_ADDR); // create the MCP4725 object for the DAC