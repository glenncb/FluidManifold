//
//    FILE: mcp9808.cpp
//  AUTHOR: Rob Tillaart
// VERSION: 0.4.1
// PURPOSE: Arduino Library for I2C mcp9808 temperature sensor
//    DATE: 2020-05-03
//     URL: https://github.com/RobTillaart/MCP9808_RT


#include <Arduino.h>
#include "mcp9808.h"
#include "i2c_acc.h"
#include "globals.h"

#define MCP9808_RFU     0x00
#define MCP9808_CONFIG  0x01
#define MCP9808_TUPPER  0x02
#define MCP9808_TLOWER  0x03
#define MCP9808_TCRIT   0x04
#define MCP9808_TA      0x05
#define MCP9808_MID     0x06
#define MCP9808_DID     0x07
#define MCP9808_RES     0x08


/*
    0x00 = 0000 = RFU, Reserved for Future Use (Read-Only register)
    0x01 = 0001 = Configuration register (CONFIG)
    0x02 = 0010 = Alert Temperature Upper Boundary Trip register (TUPPER)
    0x03 = 0011 = Alert Temperature Lower Boundary Trip register (TLOWER)
    0x04 = 0100 = Critical Temperature Trip register (TCRIT)
    0x05 = 0101 = Temperature register (TA)
    0x06 = 0110 = Manufacturer ID register
    0x07 = 0111 = Device ID/Revision register
    0x08 = 1000 = Resolution register
           1xxx = Reserved(1)
*/


MCP9808::MCP9808(const uint8_t address)
{
  _address = address;
  _offset  = 0;
  _status  = 0;
}


bool MCP9808::isConnected()
{
  bool stat = i2c.checkAddr(_address);
  return stat;
}


uint8_t MCP9808::getAddress()
{
  return _address;
}


void MCP9808::setConfigRegister(uint16_t configuration)
{
  writeReg16(MCP9808_CONFIG, configuration);
}


uint16_t MCP9808::getConfigRegister()
{
  return readReg16(MCP9808_CONFIG);
}


void  MCP9808::setTupper(float temperature)
{
  writeFloat(MCP9808_TUPPER, temperature);
}


float MCP9808::getTupper()
{
  return readFloat(MCP9808_TUPPER);
}


void  MCP9808::setTlower(float temperature)
{
  writeFloat(MCP9808_TLOWER, temperature);
}


float MCP9808::getTlower()
{
  return readFloat(MCP9808_TLOWER);
}


void  MCP9808::setTcritical(float temperature)
{
  writeFloat(MCP9808_TCRIT, temperature);
}


float MCP9808::getTcritical()
{
  return readFloat(MCP9808_TCRIT);
}


void MCP9808::setOffset(float offset)
{
  _offset = offset;
};


float MCP9808::getOffset()
{
  return _offset;
};


float MCP9808::getTemperature()
{
  return readFloat(MCP9808_TA) + _offset;
}


uint8_t MCP9808::getStatus()
{
  return _status;
}


uint16_t MCP9808::getManufacturerID()
{
  return readReg16(MCP9808_MID);
}


uint8_t MCP9808::getDeviceID()
{
  return readReg16(MCP9808_DID) >> 8;
}


uint8_t MCP9808::getRevision()
{
  return readReg16(MCP9808_DID) & 0xFF;
}


void MCP9808::setResolution(uint8_t resolution)
{
  if (resolution < 4) writeReg8(MCP9808_RES, resolution);
}


uint8_t MCP9808::getResolution()
{
  return readReg8(MCP9808_RES);
}


uint16_t MCP9808::getRFU()
{
  return readReg16(MCP9808_RFU);
}


////////////////////////////////////////////////////////////////
//
//  PRIVATE
//
void MCP9808::writeFloat(uint8_t reg, float f)
{
  bool neg = (f < 0.0);
  if (neg) f = -f;
  uint16_t val = uint16_t(f * 4 + 0.5) * 4;
  if (neg) val |= 0x1000;
  writeReg16(reg, val);
}


float MCP9808::readFloat(uint8_t reg)
{
  uint16_t val = readReg16(reg);
  float fval;
  ESP_LOGI("MCP9808::readFloat", "reg=0x%02X read val=0x%04X", reg, val);
  if (reg == MCP9808_TA)
  {
    _status = (val & 0xE000) >> 13;
  }
  if (val & 0x1000)         //  negative value
  {
    fval =((val & 0x0FFF) * 0.0625) - 256.0;
  }
  else
  {
    fval = (val & 0x0FFF) * 0.0625;
  }
  //ESP_LOGE("MCP9808::readFloat", "reg=0x%02X val = %04x, float val=%.2f", reg, val, fval);
  return fval;
}


void MCP9808::writeReg8(uint8_t reg, uint8_t value)
{
  if (reg > MCP9808_RES) return;      //  see p.16
  i2c.writeByteReg(_address, reg, value);
}


uint8_t MCP9808::readReg8(uint8_t reg)
{
  int value;
  if (reg > MCP9808_RES) return 0;    //  see p.16
  value = i2c.readByteReg(_address, reg);
  if(value < 0) 
  {
    ESP_LOGE("MCP9808::readReg8", "I2C read error at reg 0x%02X", reg);
    return 0;
  }
  return value & 0x0FF;
}


void MCP9808::writeReg16(uint8_t reg, uint16_t value)
{
  if (reg > MCP9808_RES) return;      //  see p.16
  i2c.writeWordReg(_address, reg, value);
}


uint16_t MCP9808::readReg16(uint8_t reg)
{
  if (reg > MCP9808_RES) return 0;    //  see p.16
  int value = i2c.readWordReg(_address, reg);
  if(value < 0) 
  {
    ESP_LOGE("MCP9808::readReg16", "I2C read error at reg 0x%02X", reg);
    return 0;
  }
  return value & 0x0FFFFL;
}

// modbus read/write driver functions
// called by the driver for reading PWM parameters
void modr_Temp(uint16_t *val, uint8_t drvaddr, uint8_t drvchan)
{
    float fv;

    if(!val)
    {
        ESP_LOGE("modr_Temp", "ERROR: val is NULL.");
        return;
    } 

    if(drvaddr > NUM_TEMP_SENSORS)
    {
        ESP_LOGE("modr_Temp", "ERROR: drvaddr %d is out of range.", drvaddr);
        return;
    }

    fv = temp[drvaddr].getTemperature();
    //ESP_LOGI("modr_Temp", "Read temperature: %.2f C", fv); 
    memcpy(val, &fv, sizeof(float));

#if MODBUS_DEBUG
    Serial.printf("modr_Temp: temperature = %f\n", fv);
#endif
}

void initTemp(void)
{
  uint8_t devid, rev;
  printf("Init: Temp sensor\n");
  ESP_LOGI("initTemp", "Initializing MCP9808 temperature sensor at I2C address 0x%02X", temp[0].getAddress());
  boardstat.bits.i2ctemp = 0;
  if(!temp[0].isConnected())
  {
    ESP_LOGE("initTemp", "MCP9808 temperature sensor not detected at I2C address 0x%02X", temp[0].getAddress());
    boardstat.bits.i2ctemp |= 1;
  }
  // bail if the temp sesnsor(s) are not present, the rest of the code will check the boardstat bit and skip any temp reads if not present
  if(boardstat.bits.i2ctemp) return;
  // set the config register to default

  temp[0].setConfigRegister(0x0000);
  // read the device id and revision
  devid = temp[0].getDeviceID();
  rev   = temp[0].getRevision();
  ESP_LOGI("initTemp", "MCP9808 Device ID 0=0x%02X, rev=0x%02X", devid, rev);
  // print initial temperature
  Serial.printf("MCP9808 temperature0=%.2f C\n", temp[0].getTemperature());
}

MCP9808 temp[NUM_TEMP_SENSORS] = { MCP9808(MCP9808_I2C_ADDR0) };

//  -- END OF FILE --

