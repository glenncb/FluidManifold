#pragma once
// simplified ADC class for the ADS124S08 ADC
// focuses on basic functions for the ADC and single ended input mode
#include <Arduino.h>
#include <stdint.h>
#include <map>
#include <utility>
#include "ADS112C04.h"

#define ADC_NUM_CHAN 4
#define NUM_ADC 1

#define ADCINTAVG 0

#define ADCWAIT 0

typedef struct {
    float accum;            // accumulator for averaging
    float avg;              // current averaged value
#if ADCINTAVG
    unsigned long uaccum;   // accumulator for integer averaging
    float uavg;             // current averaged value (unsigned)
#endif
} adcdata_t;

extern uint8_t adcfilter;

class ADC {
    private:
        ADS112C04 ads;
        float maxval[ADC_NUM_CHAN]; // user value for the max ADC code
        float minval[ADC_NUM_CHAN]; // user value for the min ADC code
        int32_t val[ADC_NUM_CHAN];  // current raw value for the ADC
        uint8_t _i2caddr;
        uint8_t chan;
    public:
        float CodeToValue(int32_t code, uint8_t ch) { float v = (float)((double)code * (maxval[ch] - minval[ch]) / 32767.0 + minval[ch]);
            return v > maxval[ch] ? maxval[ch] : (v < minval[ch] ? minval[ch] : v); };
        uint16_t ValueToCode(float value, uint8_t ch) { float v = (value < minval[ch] ? minval[ch] : (value > maxval[ch] ? maxval[ch] : value));
            return (uint16_t)((double)(v - minval[ch]) * 32767.0 / (maxval[ch] - minval[ch])); };
        ADC(void);                          // constructor
        ~ADC(void);                         // destructor
        void Init(uint8_t addr);            // initialize the ADC
        void SetMax(uint8_t ch,   float c) { if(ch < ADC_NUM_CHAN) maxval[ch] = c; };  // set the coefficient for the ADC
        float GetMax(uint8_t ch) { return (ch < ADC_NUM_CHAN) ? maxval[ch] : 0.0 ; };  // set the coefficient for the ADC
        void SetMin(uint8_t ch,   float c) { if(ch < ADC_NUM_CHAN) minval[ch] = c; };  // set the coefficient for the ADC
        float GetMin(uint8_t ch) { return (ch < ADC_NUM_CHAN) ? minval[ch] : 0.0 ; };  // set the coefficient for the ADC
        void SetChan(uint8_t ch) { chan = ch; };     // set the channel for the ADC
        uint8_t GetChan(void) { return chan; };      // set the channel for the ADC
        int32_t ReadRaw(uint8_t ch);
        int32_t ReadRaw(void) { return ReadRaw(chan); };
        float   Read(uint8_t ch) { return CodeToValue(ReadRaw(ch), ch); };
        float Read(void) { return Read(chan); };                   // read from current active channel in user units
};

void modr_ADC(uint16_t *val, uint32_t drvaddr, int8_t drvchan);
void adcAvgInit(void);
void adcBackgroundAvg(void *parameters);
void adcAvg(uint8_t adcnum, int8_t chan);
void adcFilterAvg(adcdata_t& adcdat, float val);
#if ADCINTAVG
void adcFilterUAvg(adcdata_t& adcdat, int32_t ival);
#endif

void initADC(void);

// declare the ADC object
extern ADC adc[];


// map to store the ADC data
extern std::map<std::pair<uint8_t, int8_t>, adcdata_t> adcAvgMap;