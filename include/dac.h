#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "MCP4725.h"
#include "i2c_acc.h"

#define DAC_I2C_ADDR 0x60 // I2C address for the MCP4725 DAC

typedef uint8_t CHANNEL_SELECT; // single-channel DAC; channel arg is accepted but ignored

class DAC {
    private:
        float maxval; // user value for the max DAC code
        float minval; // user value for the min DAC code
        uint16_t val; // current value for the DAC
        float CodeToValue(uint16_t code) { return (code * (maxval - minval)/ 4095.0 + minval); };
        uint16_t ValueToCode(float value) { return (uint16_t)((value - minval) * 4095.0 / (maxval - minval)); };
    public:
        DAC(void);                          // constructor
        ~DAC(void);                         // destructor
        void SetMax(float c) { maxval = c; };
        void SetMax(CHANNEL_SELECT, float c) { maxval = c; };  // channel arg ignored (single-channel DAC)
        float GetMax(void) { return maxval; };
        void SetMin(float c) { minval = c; };
        void SetMin(CHANNEL_SELECT, float c) { minval = c; };  // channel arg ignored
        float GetMin(void) { return minval; };
        void Init(void);               // initialize the DAC
        void Write(uint16_t data);   // write to current active channel in raw dac codes
        void Write(float val) { Write(ValueToCode(val)); }; // select ch and write to selected channel in user units
        uint16_t ReadRaw(void) { return val; }; // select ch and read from selected channel in raw dac codes (last written value)
        float Read(void) { return CodeToValue(ReadRaw()); }; // select ch and write to selected channel in user units
};

void modr_I2CDAC(uint16_t *val, uint32_t drvaddr, uint8_t drvchan);
void modw_I2CDAC(uint16_t *val, uint32_t drvaddr, uint8_t drvchan);

// declare the DAC object
extern DAC dac;
extern MCP4725 mcp;