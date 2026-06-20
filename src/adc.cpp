// ADS124S08 ADC top level functions
#include <Arduino.h>
#include <iostream>
#include <map>
#include "adc.h"
#include "i2c_acc.h"
#include "globals.h"
#include "modbus.h"
#include "modbusmap_gen.h"

uint8_t adcfilter = 0;

ADC::ADC(void)
{
    // initialize the spidev
    chan = 0;
    _i2caddr = 0;

    // initialize the min/max val arrays - by default set for a 0-5V range
    for(int i=0; i<ADC_NUM_CHAN; i++)
    {
        maxval[i] = 5.0;    // value when ADC is at full scale
        minval[i] = 0.0;    // value when ADC is at zero scale
        val[i] = 0;         // initialize the value to zero
    }
};

ADC::~ADC(void)
{
    // empty for now
};

// initialize the ADC
void ADC::Init(uint8_t addr)
{
    ESP_LOGI("ADC::Init", "Initializing ADC at I2C address 0x%02x.", addr);
    _i2caddr = addr;
    if (!i2c.checkAddr(addr))
    {
        ESP_LOGE("ADC::Init", "ADC not present at address 0x%02x", addr);
    }
    ads.setAddr_ADS112C04(addr);
    ads.setGain(GAIN_1);
    ads.setCnvMode(MODE_SINGLE);
    ads.setRate(RATE_20);
    ads.setVref(VREF_SUPPLY);
    ads.begin();
    ads.writeConfig();
};

// Map channel index to ADS112C04 single-ended input mux setting
static const adsInMux_t _adsMuxMap[ADC_NUM_CHAN] = { AIN0SE, AIN1SE, AIN2SE, AIN3SE };

// Read raw 16-bit ADC code for the designated channel
int32_t ADC::ReadRaw(uint8_t ch)
{
    if (ch >= ADC_NUM_CHAN)
    {
        ESP_LOGE("ADC::ReadRaw", "Invalid channel %d", ch);
        return -666;
    }

    if (chan != ch)
    {
        ads.setInMux(_adsMuxMap[ch]);
        ads.writeConfig();
        chan = ch;
    }

    ads.setInMux(_adsMuxMap[ch]);
    ads.writeConfig();
    chan = ch;

    int32_t data = (int32_t) ads.Measure_Differential();
    //ESP_LOGI("ADC::ReadRaw", "ch %d raw = %d", ch, data);
    return data;
};

// modbus read/write driver functions
// called by the driver for reading PWM parameters
void modr_ADC(uint16_t *val, uint32_t drvaddr, int8_t drvchan)
{
    float fv=0.0;
    uint32_t uv;
    uint16_t u16val;
    double dv=0.0;
    bool rawread = (drvchan<0);

    ESP_LOGI("modr_ADC", "modr_ADC called with drvaddr=%d, drvchan=%d, rawread=%d", drvaddr, drvchan, rawread);
    // make drvchan positive — negative value is the raw-read sentinel, channel = abs(drvchan)
    drvchan = (drvchan < 0) ? -drvchan : drvchan;

    if(!val)
    {
        ESP_LOGE("modr_ADC", "ERROR: val is NULL.");
        return;
    } 

    // use the drvaddr field to select which ADC to read
    // use the drvchan field to select which ADC channel to read for the selected ADC

    // check if we are in averaging mode and the value is already stored in the AvgMap
    if(adcfilter>0 && adcAvgMap.find(std::make_pair(static_cast<uint8_t>(drvaddr), static_cast<int>(drvchan))) != adcAvgMap.end())
    {
        fv = adcAvgMap[std::make_pair(drvaddr, drvchan)].avg;
        Serial.printf("modr_ADC: channel %d = %.2f (filter = %d)\n", drvchan, fv, adcfilter);
    }
    else // if it does not, then perform a normal adc read
    {
        uv = adc[drvaddr].ReadRaw((uint8_t) drvchan);
        if(!rawread) // negative channel number indicates we want to perform a raw read
        {
            dv = adc[drvaddr].CodeToValue(uv, drvchan);
#if MODBUS_DEBUG
            fv = (float) dv;
            Serial.printf("modr_ADC: channel %d = %.2f (code %ld)\n", drvchan, fv, uv);
            Serial.printf("modr_ADC: channel %d:  maxval=%f minval=%f\n", drvchan, adc[drvaddr].GetMax((uint8_t) drvchan), adc[drvaddr].GetMin((uint8_t) drvchan));
#endif
        }
        else
        {
#if MODBUS_DEBUG
            Serial.printf("modr_ADC: channel %d = code %ld\n", drvchan, uv);
#endif
        }

    }

    u16val = (uint16_t) uv;

    if(rawread)
        memcpy(val, &u16val, sizeof(uint16_t));
    else
        memcpy(val, &fv, sizeof(float));
}

void initADC(void)
{
    adc[0].Init(ADS112C04_DEFAULT_ADDRESS);
}

// compute the running average
// this is a moving window average with a window size of 2^adcfilter
// note that no storage arrays are used by this routine
void adcFilterAvg(adcdata_t& adcdat, float val)
{
    // if filtering is not enabled, just store the value
    if(adcfilter == 0)
    {
        adcdat.accum = 0.0;
        adcdat.avg = val;
    }
    else
    {
        // calculate delta from avg and add to running accumulator
        adcdat.accum = adcdat.accum + (val - adcdat.avg);
        // calculate the new average from the accumulated value
        adcdat.avg = adcdat.accum / (1<<adcfilter);
    }
}

#if ADCINTAVG
// integer version of averaging to improve performance
void adcFilterUAvg(adcdata_t& adcdat, int32_t ival, float adcmin, float adcmax)
{
    float dv;
    // if filtering is not enabled, just store the value
    if(adcfilter == 0)
    {
        adcdat.uaccum = 0;
        adcdat.uavg = ival;
    }
    else
    {
        // calculate delta from avg and add to running accumulator
        adcdat.uaccum = adcdat.uaccum + (ival - adcdat.uavg);
        // calculate the new average from the accumulated value
        adcdat.uavg = adcdat.uaccum >> adcfilter;
        dv = ((float) adcdat.uavg * (adcmax - adcmin) / 8388608.0 + adcmin);
        adcdat.avg = (float) dv;
    }
}
#endif

// ADC average routine called by the background task
void adcAvg(uint8_t adcnum, int8_t chan)
{
    int32_t uv;
    float fv;
    double dv;

    xSemaphoreTake(i2cInUse, portMAX_DELAY);
    uv = adc[adcnum].ReadRaw((uint8_t) chan);
    xSemaphoreGive(i2cInUse);
    
    // FIXME - should we calc the average in integer and then covert to float instead?
#if ADCINTAVG
    adcFilterUAvg(adcAvgMap[std::make_pair(adcnum, chan)], uv, adc[adcnum].GetMin((uint8_t) chan), adc[adcnum].GetMax((uint8_t) chan));
#else
    // convert the raw value to a float
    dv = adc[adcnum].CodeToValue(uv, (uint8_t) chan);
    fv = (float) dv;

    // update the average value
    adcFilterAvg(adcAvgMap[std::make_pair(adcnum, chan)], fv);
#endif
}

// declare the ADC object
ADC adc[NUM_ADC];
bool adcBackgroundReadEn = false;
SemaphoreHandle_t adcBackgndReadMutex = NULL;
// map to store the ADC data
std::map<std::pair<uint8_t, int8_t>, adcdata_t> adcAvgMap;