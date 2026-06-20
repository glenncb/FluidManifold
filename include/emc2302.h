#pragma once

// modified vewrsion of Kiat AWDSA EMC2302 library from https://github.com/kiatAWDSA/EMC2302
// forked to use standard I2C Wire Arduino library
/*********************************************************************************
Arduino Library for the Microchip EMC2302 RPM-based PWM fan controller.
I2C communication is performed using a custom I2C library.

Pg 12 of datasheet: The SMBus/I2C address is set at 0101_111(r/w)b (aka 47 or 0x2F)

Copyright (C) 2019 Soon Kiat Lau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*********************************************************************************/
#include <Arduino.h>
#include <Wire.h>

// Fan specifications
// Noctua NF-A6x25 5V PWM fan (3000 rpm max, 29.2 m3/h)
#define FAN_NOCTUA_NFA6X25 1
// Noctua NF-A6x15 5V PWM fan (3500 rpm max, 23.2 m3/h)
//#define FAN_NOCTUA_NFA6X15 1
// Wakefield Vette DC0601005U2B 5V PWM fan (4900 rpm max, 33.8 m3/h)
//#define FAN_WAKEFIELD_DC0601005U2B 1

// Define the type of fan controller being used
#define EMC2302

#ifdef EMC2302
  #define MAXFANS 2
#else
  #ifdef EMC2303
    #define MAXFANS 3
  #else
    #ifdef EMC2305
      #define MAXFANS 5
    #else
      #define MAXFANS 1
    #endif
  #endif
#endif


#ifdef FAN_NOCTUA_NFA6X25
  #define FAN_MIN_RPM   650   // 650 RPM minimum; FANVALTACHCOUNT=0xFF gives stall threshold at ~482 RPM, 500 RPM is too close
  #define FAN_MAX_RPM   3000
  #define FAN_STALL_RPM 400   // below this RPM the fan is considered physically stalled; must be below FANVALTACHCOUNT threshold (~482 RPM)
#endif
#ifdef FAN_NOCTUA_NFA6X15
  #define FAN_MIN_RPM   500
  #define FAN_MAX_RPM   3500
  #define FAN_STALL_RPM 400
#endif
#ifdef FAN_WAKEFIELD_DC0601005U2B
  #define FAN_MIN_RPM   500
  #define FAN_MAX_RPM   4900
  #define FAN_STALL_RPM 400
#endif
#if !defined(FAN_NOCTUA_NFA6X25) && !defined(FAN_NOCTUA_NFA6X15) && !defined(FAN_WAKEFIELD_DC0601005U2B)
  #define FAN_MAX_RPM   5000
  #define FAN_MIN_RPM   500
  #define FAN_STALL_RPM 400
#endif
//#define FAN_LIM_RPM 1500

// I2C address of the fan controller
#define I2C_ADDR_FAN 0x2E

// Statuses/Errors returned by the functions in this class
typedef enum
{
  EMC2302_STATUS_OK         = 0,  // No problemo.
  EMC2302_STATUS_FAIL       = 1,  // Something went wrong.
  EMC2302_STATUS_INVALIDARG = 2   // The argument given to a function is invalid.
} EMC2302_STATUS;


class EMCFan
{
public:
  EMCFan();
  ~EMCFan();

  void setChannel(uint8_t channel) { fanchan<MAXFANS ? fanchan = channel : fanchan = 0; };
  uint8_t getChannel(void) { return fanchan; };
  EMC2302_STATUS setPWMFrequencyBase(double frequencyKHz);
  EMC2302_STATUS setPWMFrequencyDivider(uint8_t divisor);
  EMC2302_STATUS toggleControlAlgorithm(bool enable);
  EMC2302_STATUS setTachMinRPM(uint16_t minRPM);
  EMC2302_STATUS setFanPoles(uint8_t poleCount);
  EMC2302_STATUS setDriveUpdatePeriod(uint16_t periodMs);
  EMC2302_STATUS toggleRampControl(bool enable);
  EMC2302_STATUS toggleGlitchFilter(bool enable);
  EMC2302_STATUS setDerivativeMode(uint8_t modeType);
  EMC2302_STATUS setControlErrRange(uint8_t errorRangeRPM);
  EMC2302_STATUS toggleSpinUpMax(bool enable);
  EMC2302_STATUS setSpinUpDrive(uint8_t drivePercent);
  EMC2302_STATUS setSpinUpTime(uint16_t timeMs);
  EMC2302_STATUS setControlMaxStep(uint8_t stepSize);
  EMC2302_STATUS setFanMinDrive(double minDrivePercent);
  EMC2302_STATUS setMinValidRPM(uint16_t minRPM);
  EMC2302_STATUS setMaxValidRPM(uint16_t maxRPM);
  EMC2302_STATUS setRPMTarget(uint16_t targetRPM);
  uint16_t getRPMTarget(void) { return fanchan < MAXFANS ? targetRPM_[fanchan] : 0; }
  EMC2302_STATUS toggleFan(bool enable);
  EMC2302_STATUS fetchFanSpeed(void);
  uint16_t getFanSpeed(void);
  bool isAnyFanStalled(void);
  bool isFanStalled(void);
  bool isFanOff(void);

private:
  // Pg 12 of datasheet : The SMBus / I2C address is set at 0101_111(r / w)b(aka 47 or 0x2F)
  const uint8_t I2C_ADDRESS = I2C_ADDR_FAN;

  uint8_t fanchan = 0; // select either channel 0 or 1 for EMC2302 dual fan controller

  // Assume that we use internal clock for tachometer
  const uint16_t TACHO_FREQUENCY = 32768;

  // 2-byte to be written to tacho target register to turn off fan
  const uint16_t TACHO_OFF = 0x1FFF << 3; // 1111 1111 1111 1000

  /******************************
   *     List of registers      *
   ******************************/
  const uint8_t EMC2302_REG_CONFIG               = 0x20;
  const uint8_t EMC2302_REG_FANSTAT              = 0x24;
  const uint8_t EMC2302_REG_FANSTALLSTAT         = 0x25;
  const uint8_t EMC2302_REG_FANSPINSTAT          = 0x26;
  const uint8_t EMC2302_REG_DRIVEFAILSTAT        = 0x27;
  const uint8_t EMC2302_REG_PWMBASEFREQ          = 0x2D;
  // Fan specific registers (for channel 0 and 1, offset by 0x10)
  const uint8_t EMC2302_REG_FANSETTING           = 0x30;
  const uint8_t EMC2302_REG_PWMDIVIDE            = 0x31;
  const uint8_t EMC2302_REG_FANCONFIG1           = 0x32;
  const uint8_t EMC2302_REG_FANCONFIG2           = 0x33;
  const uint8_t EMC2302_REG_FANSPINUP            = 0x36;
  const uint8_t EMC2302_REG_FANMAXSTEP           = 0x37;
  const uint8_t EMC2302_REG_FANMINDRIVE          = 0x38;
  const uint8_t EMC2302_REG_FANVALTACHCOUNT      = 0x39;
  const uint8_t EMC2302_REG_TACHTARGETLSB        = 0x3C;
  const uint8_t EMC2302_REG_TACHTARGETMSB        = 0x3D;
  const uint8_t EMC2302_REG_TACHREADMSB          = 0x3E;
  const uint8_t EMC2302_REG_TACHREADLSB          = 0x3F;

  /* Registers that can have values written directly into them (i.e. the entire register is meant for a single number):
      EMC2302_REG_FANSETTING
      EMC2302_REG_PWMDIVIDE
      EMC2302_REG_FANMAXSTEP (Max 0b00111111 or 0x3F or 63)
      EMC2302_REG_FANMINDRIVE
      EMC2302_REG_FANVALTACHCOUNT (The final value from this register is 32 x (value in register))
      EMC2302_REG_TACHTARGETLSB and EMC2302_REG_TACHTARGETMSB (MUST write both LSB and MSB, with LSB written before MSB)
  */

  const uint8_t EMC2302_REG_FANSTAT_FNSTL      = 0x01;
  const uint8_t EMC2302_REG_FANSTAT_FNSPIN     = 0x02;
  const uint8_t EMC2302_REG_FANSTAT_DVFAIL     = 0x04;

  // EMC2302_REG_PWMBASEFREQ
  const uint8_t EMC2302_REG_PWMBASEFREQ_26KHZ  = 0x00;
  const uint8_t EMC2302_REG_PWMBASEFREQ_19KHZ  = 0x01;
  const uint8_t EMC2302_REG_PWMBASEFREQ_4KHZ   = 0x02;
  const uint8_t EMC2302_REG_PWMBASEFREQ_2KHZ   = 0x03;

  // EMC2302_REG_FANCONFIG1
  const uint8_t EMC2302_REG_FANCONFIG1_RPMCONTROL    = 0x80;
  const uint8_t EMC2302_REG_FANCONFIG1_MINRPM_CLEAR    = ~0x60;
  const uint8_t EMC2302_REG_FANCONFIG1_MINRPM_500    = 0x00;
  const uint8_t EMC2302_REG_FANCONFIG1_MINRPM_1000   = 0x20;
  const uint8_t EMC2302_REG_FANCONFIG1_MINRPM_2000   = 0x40;
  const uint8_t EMC2302_REG_FANCONFIG1_MINRPM_4000   = 0x60;
  const uint8_t EMC2302_REG_FANCONFIG1_FANPOLES_CLEAR  = ~0x18;
  const uint8_t EMC2302_REG_FANCONFIG1_FANPOLES_1    = 0x00;
  const uint8_t EMC2302_REG_FANCONFIG1_FANPOLES_2    = 0x08;
  const uint8_t EMC2302_REG_FANCONFIG1_FANPOLES_3    = 0x10;
  const uint8_t EMC2302_REG_FANCONFIG1_FANPOLES_4    = 0x18;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_CLEAR    = ~0x07;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_100    = 0x00;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_200    = 0x01;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_300    = 0x02;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_400    = 0x03;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_500    = 0x04;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_800    = 0x05;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_1200   = 0x06;
  const uint8_t EMC2302_REG_FANCONFIG1_UPDATE_1600   = 0x07;

  // EMC2302_REG_FANCONFIG2
  const uint8_t EMC2302_REG_FANCONFIG2_RAMPCONTROL   = 0x40;
  const uint8_t EMC2302_REG_FANCONFIG2_GLITCHFILTER  = 0x20;
  const uint8_t EMC2302_REG_FANCONFIG2_DEROPT_CLEAR    = ~0x18;
  const uint8_t EMC2302_REG_FANCONFIG2_DEROPT_NONE   = 0x00;
  const uint8_t EMC2302_REG_FANCONFIG2_DEROPT_BASIC  = 0x08;
  const uint8_t EMC2302_REG_FANCONFIG2_DEROPT_STEP   = 0x10;
  const uint8_t EMC2302_REG_FANCONFIG2_DEROPT_BOTH   = 0x18;
  const uint8_t EMC2302_REG_FANCONFIG2_ERRRANGE_CLEAR  = ~0x06;
  const uint8_t EMC2302_REG_FANCONFIG2_ERRRANGE_0    = 0x00;
  const uint8_t EMC2302_REG_FANCONFIG2_ERRRANGE_50   = 0x02;
  const uint8_t EMC2302_REG_FANCONFIG2_ERRRANGE_100  = 0x04;
  const uint8_t EMC2302_REG_FANCONFIG2_ERRRANGE_200  = 0x06;

  // EMC2302_REG_FANSPINUP
  const uint8_t EMC2302_REG_FANSPINUP_NOKICK           = 0x20;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_CLEAR      = ~0x1C;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_30       = 0x00;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_35       = 0x04;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_40       = 0x08;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_45       = 0x0C;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_50       = 0x10;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_55       = 0x14;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_60       = 0x18;
  const uint8_t EMC2302_REG_FANSPINUP_SPINLVL_65       = 0x1C;
  const uint8_t EMC2302_REG_FANSPINUP_SPINUPTIME_CLEAR   = ~0x03;
  const uint8_t EMC2302_REG_FANSPINUP_SPINUPTIME_250   = 0x00;
  const uint8_t EMC2302_REG_FANSPINUP_SPINUPTIME_500   = 0x01;
  const uint8_t EMC2302_REG_FANSPINUP_SPINUPTIME_1000  = 0x02;
  const uint8_t EMC2302_REG_FANSPINUP_SPINUPTIME_2000  = 0x03;

  // EMC2302_REG_FANMAXSTEP
  const uint8_t EMC2302_REG_FANMAXSTEP_MAX = 0b00111111;

  uint16_t tachMaxRPM_[MAXFANS];
  uint16_t tachMinRPM_[MAXFANS];
  uint8_t tachMinRPMMultiplier_[MAXFANS];
  uint8_t tachFanPolesMultiplier_[MAXFANS]; // To avoid doubles, this is actually multiplied by 2 to make it an integer.
  uint8_t fanPoleCount_[MAXFANS];
  uint16_t targetTachCount_[MAXFANS];
  bool fanEnabled_[MAXFANS];
  uint16_t fanSpeed_[MAXFANS];
  uint16_t targetRPM_[MAXFANS];

  EMC2302_STATUS writeRegisterBits(uint8_t registerAddress, uint8_t clearingMask, uint8_t byteToWrite);
  EMC2302_STATUS writeTachoTarget(uint16_t tachoTarget);
};

extern EMCFan fan;
void initFan(void);

#define FANDRVR_RPMTARGET 0
#define FANDRVR_RPM 1

// modbus drivers
uint16_t modr_Fan(uint16_t *val, uint8_t drvaddr, uint8_t drvchan);
void modw_Fan(uint16_t *val, uint8_t drvaddr, uint8_t drvchan);
