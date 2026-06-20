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

#include "emc2302.h"
#include "i2c_acc.h"
#include "globals.h"

EMCFan::EMCFan(void)
{
  // Base config for a fan with 2 poles and 500 min RPM.
  // Fan starts in off state.
  for(int i=0;i<MAXFANS;i++)
  {
    tachMinRPMMultiplier_[i]    = 1;    // default to 500 min RPM
    tachMaxRPM_[i]              = 10000;  // default to 10,000 max RPM
    tachMinRPM_[i]              = 500;  // default to 500 min RPM
    tachFanPolesMultiplier_[i]  = 2;    // 5 edges / 2 pole fan -> (edges-1)/poles = multiplier of 2
    fanPoleCount_[i]            = 2;    // default to 2 poles
    fanSpeed_[i]                = 0;
    targetTachCount_[i]         = TACHO_OFF;
    fanEnabled_[i]              = false;
    fanchan = 0;
  }
}

EMCFan::~EMCFan()
{
}

// Set the base frequency of the PWM output, which could be divided further by setPWMFrequencyDivider().
// The function is written such that any frequency below the next higher base frequency
// will automatically be converted to the previous frequency.
EMC2302_STATUS EMCFan::setPWMFrequencyBase(double frequencyKHz)
{
  uint8_t writeByte;
  if (frequencyKHz < 4.882)
  {
    writeByte = EMC2302_REG_PWMBASEFREQ_2KHZ;
  }
  else if (frequencyKHz < 19.531)
  {
    writeByte = EMC2302_REG_PWMBASEFREQ_4KHZ;
  }
  else if (frequencyKHz < 26)
  {
    writeByte = EMC2302_REG_PWMBASEFREQ_19KHZ;
  }
  else
  {
    writeByte = EMC2302_REG_PWMBASEFREQ_26KHZ;
  }

  if(i2c.writeByteReg(EMC2302_REG_PWMBASEFREQ, writeByte) != -1)
  {
    return EMC2302_STATUS_OK;
  }
  else
  {
    return EMC2302_STATUS_FAIL;
  }
}

// 
EMC2302_STATUS EMCFan::setPWMFrequencyDivider(uint8_t divisor)
{
  if(i2c.writeByteReg(EMC2302_REG_PWMDIVIDE + fanchan*0x10, divisor) != -1)
  {
    return EMC2302_STATUS_OK;
  }
  else
  {
    return EMC2302_STATUS_FAIL;
  }
}

// Toggles the RPM-based Fan Speed Control Algorithm, whereby the fan speed will be controlled
// by the EMC2302 using a PID algorithm based on the target tachometer reading given by user.
EMC2302_STATUS EMCFan::toggleControlAlgorithm(bool enable)
{
  if (enable)
  {
    return writeRegisterBits(EMC2302_REG_FANCONFIG1 + fanchan*0x10, ~EMC2302_REG_FANCONFIG1_RPMCONTROL, EMC2302_REG_FANCONFIG1_RPMCONTROL);
  }
  else
  {
    return writeRegisterBits(EMC2302_REG_FANCONFIG1 + fanchan*0x10, ~EMC2302_REG_FANCONFIG1_RPMCONTROL, 0);
  }
}

// set the upper RPM limit for the fan - requests above this threshold will be clamped to max
EMC2302_STATUS EMCFan::setMaxValidRPM(uint16_t maxRPM)
{
  tachMaxRPM_[fanchan] = maxRPM;
  return EMC2302_STATUS_OK;
}
// Since the tachometer register on the EMC2302 has an upper limit, it is necessary to apply multipliers
// for fans with high RPMs. This is done by adjusting the minimum RPM expected for the fan.
// The function is written such that the min RPM will be forced to the closest lower RPM.
EMC2302_STATUS EMCFan::setTachMinRPM(uint16_t minRPM)
{
  uint8_t writeByte;
  if (minRPM < 1000)
  {
    writeByte = EMC2302_REG_FANCONFIG1_MINRPM_500;
    tachMinRPMMultiplier_[fanchan] = 1;
  }
  else if (minRPM < 2000)
  {
    writeByte = EMC2302_REG_FANCONFIG1_MINRPM_1000;
    tachMinRPMMultiplier_[fanchan] = 2;
  }
  else if (minRPM < 4000)
  {
    writeByte = EMC2302_REG_FANCONFIG1_MINRPM_2000;
    tachMinRPMMultiplier_[fanchan]   = 4;
  }
  else
  {
    writeByte = EMC2302_REG_FANCONFIG1_MINRPM_4000;
    tachMinRPMMultiplier_[fanchan] = 8;
  }

  return writeRegisterBits(EMC2302_REG_FANCONFIG1 + fanchan*0x10, EMC2302_REG_FANCONFIG1_MINRPM_CLEAR, writeByte);
}

// The number of fan poles would also affect the tachometer counting because
// different fans generate different amount of pulses per rotation. Most fans
// are 2 poles, but this info should be obtained directly from the fan datasheet.
EMC2302_STATUS EMCFan::setFanPoles(uint8_t poleCount)
{
  uint8_t writeByte;

  // To avoid doubles, we first multiply the fan pole multiplier by 2 to make it an integer.
  // It will be divided by 2 in the equations relating RPM to tachometer readings.
  switch (poleCount)
  {
  case 1:
    writeByte = EMC2302_REG_FANCONFIG1_FANPOLES_1;
    tachFanPolesMultiplier_[fanchan] = 1;
    break;
  case 2:
    writeByte = EMC2302_REG_FANCONFIG1_FANPOLES_2;
    tachFanPolesMultiplier_[fanchan] = 2;
    break;
  case 3:
    writeByte = EMC2302_REG_FANCONFIG1_FANPOLES_3;
    tachFanPolesMultiplier_[fanchan] = 3;
    break;
  case 4:
    writeByte = EMC2302_REG_FANCONFIG1_FANPOLES_4;
    tachFanPolesMultiplier_[fanchan] = 4;
  default:
    return EMC2302_STATUS_INVALIDARG;
    break;
  }

  return writeRegisterBits(EMC2302_REG_FANCONFIG1 + fanchan*0x10, EMC2302_REG_FANCONFIG1_FANPOLES_CLEAR, writeByte);
}

// Adjusts the period between subsequent PWM drive updates.
// This is only used if toggleRampControl() was enabled.
// The function is written such that the period will be forced to the closest lower period.
EMC2302_STATUS EMCFan::setDriveUpdatePeriod(uint16_t periodMs)
{
  uint8_t writeByte;
  if (periodMs < 200)
  {
    writeByte = EMC2302_REG_FANCONFIG1_UPDATE_100;
  }
  else if (periodMs < 300)
  {
    writeByte = EMC2302_REG_FANCONFIG1_UPDATE_200;
  }
  else if (periodMs < 400)
  {
    writeByte = EMC2302_REG_FANCONFIG1_UPDATE_300;
  }
  else if (periodMs < 500)
  {
    writeByte = EMC2302_REG_FANCONFIG1_UPDATE_400;
  }
  else if (periodMs < 800)
  {
    writeByte = EMC2302_REG_FANCONFIG1_UPDATE_500;
  }
  else if (periodMs < 1200)
  {
    writeByte = EMC2302_REG_FANCONFIG1_UPDATE_800;
  }
  else if (periodMs < 1600)
  {
    writeByte = EMC2302_REG_FANCONFIG1_UPDATE_1200;
  }
  else
  {
    writeByte = EMC2302_REG_FANCONFIG1_UPDATE_1600;
  } 

  return writeRegisterBits(EMC2302_REG_FANCONFIG1 + fanchan*0x10, EMC2302_REG_FANCONFIG1_UPDATE_CLEAR, writeByte);
}

// Toggle ramp control for DIRECT CONTROL MODE, whereby the fan speed will be increased gradually.
// Disabling this will allow fan speed to be changed instantly.
// Note that enabling the RPM-based Fan Speed Control will automatically use the
// ramp control, regardless of the status of this function.
EMC2302_STATUS EMCFan::toggleRampControl(bool enable)
{
  if (enable)
  {
    return writeRegisterBits(EMC2302_REG_FANCONFIG2 + fanchan*0x10, ~EMC2302_REG_FANCONFIG2_RAMPCONTROL, EMC2302_REG_FANCONFIG2_RAMPCONTROL);
  }
  else
  {
    return writeRegisterBits(EMC2302_REG_FANCONFIG2 + fanchan*0x10, ~EMC2302_REG_FANCONFIG2_RAMPCONTROL, 0);
  }
}

// Toggle the glitch filter, which removes high frequency noise
// from the TACH pin.
EMC2302_STATUS EMCFan::toggleGlitchFilter(bool enable)
{
  if (enable)
  {
    return writeRegisterBits(EMC2302_REG_FANCONFIG2 + fanchan*0x10, ~EMC2302_REG_FANCONFIG2_GLITCHFILTER, EMC2302_REG_FANCONFIG2_GLITCHFILTER);
  }
  else
  {
    return writeRegisterBits(EMC2302_REG_FANCONFIG2 + fanchan*0x10, ~EMC2302_REG_FANCONFIG2_GLITCHFILTER, 0);
  }
}

// Change the type of derivative used in the PID algorithm for RPM-based speed control.
// Refer to Table 5.15 at pg 31 of the datasheet.
EMC2302_STATUS EMCFan::setDerivativeMode(uint8_t modeType)
{
  uint8_t writeByte;

  switch (modeType)
  {
  case 0:
    writeByte = EMC2302_REG_FANCONFIG2_DEROPT_NONE;
    break;
  case 1:
    writeByte = EMC2302_REG_FANCONFIG2_DEROPT_BASIC;
    break;
  case 2:
    writeByte = EMC2302_REG_FANCONFIG2_DEROPT_STEP;
    break;
  case 3:
    writeByte = EMC2302_REG_FANCONFIG2_DEROPT_BOTH;
  default:
    return EMC2302_STATUS_INVALIDARG;
    break;
  }

  return writeRegisterBits(EMC2302_REG_FANCONFIG2 + fanchan*0x10, EMC2302_REG_FANCONFIG2_DEROPT_CLEAR, writeByte);
}

// Since the tachometer has an accuracy rating, it is not expected that the
// RPM readings will be constant even if the PWM drive is constant. Therefore,
// it may be desirable to tell the EMC2302 to stop changing PWM drive as long as
// the RPM reading is within a tolerance of the target. This function does that.
// The function is written such that the error range will be forced to the closest higher range.
// The argument should be a positive number.
EMC2302_STATUS EMCFan::setControlErrRange(uint8_t errorRangeRPM)
{
  uint8_t writeByte;
  if (errorRangeRPM < 0.01) // Account for doubles sometimes not being exactly 0
  {
    writeByte = EMC2302_REG_FANCONFIG2_ERRRANGE_0;
  }
  else if (errorRangeRPM <= 50)
  {
    writeByte = EMC2302_REG_FANCONFIG2_ERRRANGE_50;
  }
  else if (errorRangeRPM <= 100)
  {
    writeByte = EMC2302_REG_FANCONFIG2_ERRRANGE_100;
  }
  else
  {
    writeByte = EMC2302_REG_FANCONFIG2_ERRRANGE_200;
  }

  return writeRegisterBits(EMC2302_REG_FANCONFIG2 + fanchan*0x10, EMC2302_REG_FANCONFIG2_ERRRANGE_CLEAR, writeByte);
}

// Toggle max spin up, whereby the fan is set to 100% duty cycle for 1/4th of the
// time during the spin up routine.
EMC2302_STATUS EMCFan::toggleSpinUpMax(bool enable)
{
  if (enable)
  {
    return writeRegisterBits(EMC2302_REG_FANSPINUP + fanchan*0x10, ~EMC2302_REG_FANSPINUP_NOKICK, EMC2302_REG_FANSPINUP_NOKICK);
  }
  else
  {
    return writeRegisterBits(EMC2302_REG_FANSPINUP + fanchan*0x10, ~EMC2302_REG_FANSPINUP_NOKICK, 0);
  }
}

// Set the drive level that should be used during the spin up routine.
// The function is written such that the drive will be forced to the closest lower drive.
EMC2302_STATUS EMCFan::setSpinUpDrive(uint8_t drivePercent)
{
  uint8_t writeByte;
  if (drivePercent < 35)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINLVL_30;
  }
  else if (drivePercent < 40)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINLVL_35;
  }
  else if (drivePercent < 45)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINLVL_40;
  }
  else if (drivePercent < 50)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINLVL_45;
  }
  else if (drivePercent < 55)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINLVL_50;
  }
  else if (drivePercent < 60)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINLVL_55;
  }
  else if (drivePercent < 65)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINLVL_60;
  }
  else
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINLVL_65;
  }

  return writeRegisterBits(EMC2302_REG_FANSPINUP + fanchan*0x10, EMC2302_REG_FANSPINUP_SPINLVL_CLEAR, writeByte);
}

// Determine the duration of the spin up routine.
// The function is written such that the time will be forced to the closest shorter time.
EMC2302_STATUS EMCFan::setSpinUpTime(uint16_t timeMs)
{
  uint8_t writeByte;
  if (timeMs < 500)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINUPTIME_250;
  }
  else if (timeMs < 1000)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINUPTIME_500;
  }
  else if (timeMs < 2000)
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINUPTIME_1000;
  }
  else
  {
    writeByte = EMC2302_REG_FANSPINUP_SPINUPTIME_2000;
  }

  return writeRegisterBits(EMC2302_REG_FANSPINUP + fanchan*0x10, EMC2302_REG_FANSPINUP_SPINUPTIME_CLEAR, writeByte);
}

// Set the maximum change in fan drive that could be performed over a single
// update period. Maximum is 0b00111111 (aka 63 or 0x3F)
EMC2302_STATUS EMCFan::setControlMaxStep(uint8_t stepSize)
{
  if (stepSize > EMC2302_REG_FANMAXSTEP_MAX)
  {
    stepSize = EMC2302_REG_FANMAXSTEP_MAX;
  }

  if(i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_FANMAXSTEP + fanchan*0x10, stepSize) != -1)
  {
    return EMC2302_STATUS_OK;
  }
  else
  {
    return EMC2302_STATUS_FAIL;
  }
}

// Sets the minimum allowable drive for the RPM-based Fan Speed Control algorithm.
// The algorithm will not drive the fan at a level lower than this unless the
// tachometer target is specifically set to 0xFF.
// This is extremely useful for fans that would stop spinning if the PWM signal
// is low but not zero because once the PWM signal is low enough, the fan stops spinning
// and the tachometer readings become zero causing the algorithm to drive high and restart
// the fan, but now the tachometer is above the target. The fan is then driven to a halt again
// and this process is repeated indefinitely, causing the fan to on-off-on-off......
// Having a minimum drive prevents this from happening.
EMC2302_STATUS EMCFan::setFanMinDrive(double minDrivePercent)
{
  // Convert the percent to byte format
  uint8_t writeByte = (uint8_t) (constrain(minDrivePercent, 0, 100) / 100 * 255);

  if(i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_FANMINDRIVE + fanchan*0x10, writeByte) != -1)
  {
    return EMC2302_STATUS_OK;
  }
  else
  {
    return EMC2302_STATUS_FAIL;
  }
}

// Sets the minimum RPM which is checked at the end of the spin up routine to decide if the fan is actually
// moving or if it is stalled.
// Internally, the function converts the min RPM to tachometer count that will be written
// to the appropriate register.
// NOTE: this value shouldn't be the absolute minimum RPM because it only serves as a check
// for the spin up routine. Absolute minimum speed should be set at setFanMinDrive(), although
// that function accepts percentage, not RPM.
// Also NOTE: the min value is dependent on what was set in setTachMinRPM(). This function will automatically
// increase the RPM to the lower limit if the given RPM is lower than the one set in setTachMinRPM().
EMC2302_STATUS EMCFan::setMinValidRPM(uint16_t minRPM)
{
  // Ensure the given min RPM is not below the limits of the tachometer.
  uint16_t tachMinRPM;

  tachMinRPM_[fanchan] = minRPM;  // used to clamp the min RPM when setting the target RPM

  // determine which range the min RPM based on multiplier
  switch (tachMinRPMMultiplier_[fanchan])
  {
    case 1:
      tachMinRPM = 500;
      break;
    case 2:
      tachMinRPM = 1000;
      break;
    case 3:
      tachMinRPM = 2000;
      break;
    case 4:
    default:
      tachMinRPM = 4000;
      break;
  }

  if (minRPM < tachMinRPM)
  {
    minRPM = tachMinRPM;
  }

  // FANVALTACHCOUNT detects a completely stalled fan after spin-up — it must be set BELOW
  // the minimum operating speed, not equal to it. Setting it to minRPM puts the threshold
  // at 7840 counts (500 RPM), but 500 RPM itself = 7864 counts > 7840, so the EMC2302
  // forces full-scale drive whenever the fan targets 500 RPM, causing oscillation.
  // 0xFF gives threshold = 32*255 = 8160 counts (~482 RPM), safely below FAN_MIN_RPM.
  uint16_t maxTachCount_ = 0xFF;

  if(i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_FANVALTACHCOUNT + fanchan*0x10, maxTachCount_) != -1)
  {
    return EMC2302_STATUS_OK;
  }
  else
  {
    return EMC2302_STATUS_FAIL;
  }
}

// Based on the given target RPM, calculate the appropriate target tachometer reading and
// write it to the appropriate register.
// TODO: need sanity check for targetRPM to ensure it doesn't cause the calculation to overflow
// the max value of uint16_t (65535).
EMC2302_STATUS EMCFan::setRPMTarget(uint16_t targetRPM)
{
  ESP_LOGI("EMC2302", "setRPMTarget: targetRPM=%d\n", targetRPM);
  if(targetRPM == 0) // special case for turning off the fan
  {
    fanEnabled_[fanchan] = false;
    return writeTachoTarget(TACHO_OFF);
  }
  else if(targetRPM < tachMinRPM_[fanchan]) // clamp to min RPM
  {
    targetRPM = tachMinRPM_[fanchan];
  }
  else if(targetRPM > tachMaxRPM_[fanchan]) // clamp to max RPM
  {
    targetRPM = tachMaxRPM_[fanchan];
  }
  targetRPM_[fanchan] = targetRPM;
  fanEnabled_[fanchan] = true;
  // To avoid doubles, the fan pole multiplier was multiplied by 2 to make it an integer.
  // Here, we divide it (and the -1 in the bracket) by 2 to bring it back to its proper value.
  targetTachCount_[fanchan] = (uint16_t) ((float) (60.0 * (tachMinRPMMultiplier_[fanchan]) * TACHO_FREQUENCY * (tachFanPolesMultiplier_[fanchan]) / targetRPM));
  return writeTachoTarget(targetTachCount_[fanchan]);
}

// Turn the fan on (to the most recently known target RPM) or turn it off
EMC2302_STATUS EMCFan::toggleFan(bool enable)
{
  fanEnabled_[fanchan] = enable;
  if (enable)
  {
    return writeTachoTarget(targetTachCount_[fanchan]);
  }
  else
  {
    return writeTachoTarget(TACHO_OFF);
  }
}

// report if the fan is currently off by checking for TACH TARGET MSByte of all 1's
bool EMCFan::isFanOff(void)
{
  int val;

  if(val = i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_TACHTARGETMSB + fanchan*0x10) >= 0)
  {
    return (((val & 0xFF) == 0x0FF) || (!fanEnabled_[fanchan]));
  }
  return false;
}

// report if the fan is currently in stalled state but not in off state
bool EMCFan::isAnyFanStalled(void)
{
  int val;

  bool alloff=true;

  // if all fans are off, return false
  for(int i=0; i<MAXFANS; i++)
  {
    if(fanEnabled_[i]) 
    {
      alloff = false;
      break;
    }
  } 
  if(alloff) return false;

  // perform a dummy read of the stall register to clear any pending errors
  i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_FANSTALLSTAT);
  i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_FANSTALLSTAT, 0x00);
  // write the fan status register to clear any pending errors
  i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_FANSTAT, 0x00);
  // if still reporting a stall on the global fanstall bit, return true
  if((val = i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_FANSTAT)) >= 0)
  {
    return ((val & EMC2302_REG_FANSTAT_FNSTL) != 0);
  }
  return false;
}

// report of the specific fan channel is currently in stalled state but not in off state
bool EMCFan::isFanStalled(void)
{
  int val;

  if(isFanOff()) return false;

  // perform a dummy read of the stall register to clear any pending errors
  i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_FANSTALLSTAT);
  i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_FANSTALLSTAT, 0x00);
  // write the fan status register to clear any pending errors
  i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_FANSTAT, 0x00);
  // if still reporting a stall, check which fan is stalled
  if((val = i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_FANSTAT)) >= 0)
  {
    // a fan is stalled - check if the selected fan is stalled
    uint8_t chanmask = (0x01 << fanchan) & 0x1F;
    val = i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_FANSTALLSTAT);
    return ((val & chanmask) != 0);
  }
  return false;
}

// Obtain the tachometer reading, convert to RPM, and store in a private variable.
// Get the fan speed (RPM) by calling getFanSpeed().
EMC2302_STATUS EMCFan::fetchFanSpeed()
{
  int val;
  // read the upper byte of the fan status
  if((val = i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_TACHREADMSB + fanchan*0x10)) >= 0) 
  {
    uint16_t tachoCount = (val & 0x0FF) << 8;
    if((val = i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_TACHREADLSB + fanchan*0x10)) >= 0) 
    {
      tachoCount |= val & 0x0FF;
      tachoCount = tachoCount >> 3;

      // EMC2302 saturates TACHREAD to 0x1FFF when no tach pulses are received (fan absent or stopped).
      // Check this before computing RPM to avoid reporting the ~480 RPM artifact.
      if(tachoCount >= 0x1FFF || isFanOff())
      {
        fanSpeed_[fanchan] = 0;
        return EMC2302_STATUS_OK;
      }

      // To avoid doubles, the fan pole multiplier was multiplied by 2 to make it an integer.
      fanSpeed_[fanchan] = (uint16_t) ((float) (60.0 * (tachMinRPMMultiplier_[fanchan]) * TACHO_FREQUENCY * (tachFanPolesMultiplier_[fanchan]) / tachoCount));

      ESP_LOGI("EMCFan::fetchFanSpeed", "Fan %d tachoCount=%d, speed=%d rpm\n", fanchan, tachoCount, fanSpeed_[fanchan]);
      return EMC2302_STATUS_OK;
    }
    else
    {
      return EMC2302_STATUS_FAIL;
    }
  }
  else
  {
    return EMC2302_STATUS_FAIL;
  }
  // should never reach this
  return EMC2302_STATUS_OK;
}

uint16_t EMCFan::getFanSpeed()
{
  return fanSpeed_[fanchan];
}

// Writes specific bits in the given register, such that the other bits in the register
// are unaffected. This is done by reading the register first, masking the appropriate
// bits, and then writing this modified byte into the register.
EMC2302_STATUS EMCFan::writeRegisterBits(uint8_t registerAddress, uint8_t clearingMask, uint8_t byteToWrite)
{
  int val;

  if((val = i2c.readByteReg(I2C_ADDRESS, registerAddress)) >= 0)
  {
    uint8_t registerContents = val & 0x0FF;
    registerContents &= clearingMask; // Reset the bits at the location of interest
    registerContents |= byteToWrite;  // Write bits to the location of interest

    if(i2c.writeByteReg(I2C_ADDRESS, registerAddress, registerContents) != -1)
    {
      return EMC2302_STATUS_OK;
    }
    else
    {
      return EMC2302_STATUS_FAIL;
    }
  }
  else
  {
    return EMC2302_STATUS_FAIL;
  }
}

EMC2302_STATUS EMCFan::writeTachoTarget(uint16_t tachoTarget)
{
  uint8_t tachCountLSB = (tachoTarget << 3) & 0xF8;
  uint8_t tachCountMSB = (tachoTarget >> 5) & 0xFF;
  uint8_t check;

  ESP_LOGI("EMC2302", "writeTachoTarget: Fan %d, target: %x, tachCountLSB=%x, tachCountMSB=%x\n", fanchan, tachoTarget, tachCountLSB, tachCountMSB);
  // The low byte must be written before the high byte, because
  // the target is officially changed once the high byte is written (pg 36 of datasheet).
  if(i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_TACHTARGETLSB + fanchan*0x10, tachCountLSB) != -1)
  {
    if(i2c.writeByteReg(I2C_ADDRESS, EMC2302_REG_TACHTARGETMSB + fanchan*0x10, tachCountMSB) != -1)
    {
#ifdef EMCREADCHECK
      // perform a readback check for target tach count as long as not TACH_OFF
      if(tachoTarget != TACHO_OFF)
      {
        // after wrting the MSB, the LSB and MSB regs are updated, read back to confirm
        check = i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_TACHTARGETLSB + fanchan*0x10);
        if(check != tachCountLSB)
        {
          ESP_LOGE("EMCFan::writeTachoTarget", "Readback of tachCountLSB did not match for fan %d: wrote %x read %x", fanchan, tachCountLSB, check);
        }
        check = i2c.readByteReg(I2C_ADDRESS, EMC2302_REG_TACHTARGETMSB + fanchan*0x10);
        if(check != tachCountMSB)
        {
          ESP_LOGE("EMCFan::writeTachoTarget", "Readback of tachCountMSB did not match for fan %d: wrote %x read %x", fanchan, tachCountMSB, check);
        }
      }
#endif
      return EMC2302_STATUS_OK;
    }
    else
    {
      return EMC2302_STATUS_FAIL;
    }
  }
  else
  {
    return EMC2302_STATUS_FAIL;
  }
}

EMCFan fan;

void initFan(void)
{
  int rpmtarget, rpm;
  int stat;

  printf("Init: Fan controller\n");
  // check for fan present on I2C bus
  if(i2c.checkAddr(I2C_ADDR_FAN) != true)
  {
    Serial.printf("fanInit: EMC2302 fan controller not present on I2C bus at address 0x%02x\n", I2C_ADDR_FAN);
    boardstat.bits.i2cfan = 1;
    return;
  }

  // mark fan controller as present
  boardstat.bits.i2cfan = 0;
  Serial.printf("initFan: Initializing fan controller and turning fan on initially\n");
  boardstat.bits.fanstall = 0; // mark fan as stalled

  // setup fan attributes for all the fans connected to the controller
  for(int i=0; i<MAXFANS; i++)
  {
    fan.setChannel(i);
    fan.setFanPoles(2);               // 2 pole
    fan.setMaxValidRPM(FAN_MAX_RPM);  // max RPM
    fan.setTachMinRPM(FAN_MIN_RPM);   // must be called before setMinValidRPM to set multiplier
    fan.setMinValidRPM(FAN_MIN_RPM);  // min RPM; FANVALTACHCOUNT set to 0xFF (~482 RPM stall threshold)
    fan.setFanMinDrive(18.0);         // 18% min PWM; keeps fan above 482 RPM hardware stall threshold during deceleration
    fan.toggleControlAlgorithm(true); // turn on automatic fan control by RPM target
    fan.setRPMTarget(2000);
    fan.toggleFan(true);             // turn fan on
    delay(1500); // wait for fan to spin up
    fan.fetchFanSpeed();             // fetch the current fan speed
    rpm = fan.getFanSpeed();         // get the current fan speed
    if(rpm < FAN_MIN_RPM)
    {
      Serial.printf("initFan: Fan %d did not spin up, current speed is %d RPM\n", i, rpm);
      boardstat.bits.fanstall |= 1<<i;  // mark fan as stalled
    }
    else
    {
      boardstat.bits.fanstall &= ~(1<<i); // mark fan as running
    }
  }
}

// modbus drivers
uint16_t modr_Fan(uint16_t *val, uint8_t drvaddr, uint8_t drvchan)
{
  if(!val)
  {
    ESP_LOGE("modr_Fan", "val pointer is null");
    return 0;
  }

  if(drvchan >= MAXFANS)
  {
    ESP_LOGE("modr_Fan", "invalid drvchan %d - must be 0..%d", drvchan, MAXFANS-1);
    return 0;
  } 

  // otherwise return the requested value
  switch (drvaddr)
  {
    case FANDRVR_RPMTARGET: // get current fan setpoint RPM
      fan.setChannel(drvchan);
      *val = fan.getRPMTarget();
      break;
    case FANDRVR_RPM: // get current fan speed (actual RPM)
      fan.setChannel(drvchan);
      fan.fetchFanSpeed();
      *val = fan.getFanSpeed();
      break;
    default:
      ESP_LOGE("modr_Fan", "invalid drvaddr %d", drvaddr);
      break;
  }
  Serial.printf("modr_Fan addr %d chan %d, val=%d\n", drvaddr, drvchan, *val);
  return *val;
}
void modw_Fan(uint16_t *val, uint8_t drvaddr, uint8_t drvchan)
{
  if(!val)
  {
    ESP_LOGE("modw_Fan", "val pointer is null");
    return;
  }

  if(drvchan >= MAXFANS)
  {
    ESP_LOGE("modw_Fan", "invalid drvchan %d - must be 0..%d", drvchan, MAXFANS-1);
    return;
  } 

  // otherwise return the requested value
  switch (drvaddr)
  {
    case FANDRVR_RPMTARGET: // get current fan setpoint RPM
      fan.setChannel(drvchan);
      fan.setRPMTarget(*val);
      break;
    default:
      ESP_LOGE("modr_Fan", "invalid drvaddr %d", drvaddr);
      break;
  }
  return;
}
