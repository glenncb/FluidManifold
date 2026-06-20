#pragma once
// leveraged from M5Stack ADS112C04 library
/*!
 * @brief A 16-bit ADS112C04 ADC converter library From M5Stack
 * @copyright Copyright (c) 2022 by M5Stack[https://m5stack.com]
 *
 * @Links [Unit ADC](https://docs.m5stack.com/en/unit/adc)
 * @Links [Hat ADC](https://docs.m5stack.com/en/hat/hat-adc)
 * @version  V0.0.1
 * @date  2022-07-11
 */

#include <Arduino.h>
#include <stdint.h>

/**************************************************************************
    I2C ADDRESS/BITS
**************************************************************************/
#define ADS112C04_DEFAULT_ADDRESS (0x40)  

/**************************************************************************
    CONVERSION DELAY (in mS) for slowest rate
 *************************************************************************/
#define ADS112C04_CONVERSIONDELAY (50)

//---------------------------------------------------------------------
// ADS112C04 commands
//---------------------------------------------------------------------
#define ADS112C04_CMD_RESET         (0x06)  // Reset the ADS112C04
#define ADS112C04_CMD_START         (0x08)  // Start a conversion
#define ADS112C04_CMD_POWERDOWN     (0x02)  // Stop converting and enter power down mode
#define ADS112C04_CMD_RDATA         (0x10)  // Read data once
#define ADS112C04_CMD_RREG          (0x20)  // Read register
#define ADS112C04_CMD_WREG          (0x40)  // Write register
// register numbers
#define ADS112C04_REG_CONFIG0       (0x00)  // Config register 0
#define ADS112C04_REG_CONFIG1       (0x04)  // Config register 1
#define ADS112C04_REG_CONFIG2       (0x08)  // Config register 2
#define ADS112C04_REG_CONFIG3       (0x0C)  // Config register 3

//---------------------------------------------------------------------
// ADS112C04 Configuration Register 0
//---------------------------------------------------------------------
// 7:4 Input Multiplexer Configuration (MUX)
#define ADS112C04_REG_CONFIG0_MUX_MASK (0xF0)  // Input multiplexer configuration
#define ADS112C04_REG_CONFIG0_MUX_AIN01DIFF (0x00)  // AIN0 - AIN1 (default)
#define ADS112C04_REG_CONFIG0_MUX_AIN02DIFF (0x10)  // AIN0 - AIN2
#define ADS112C04_REG_CONFIG0_MUX_AIN03DIFF (0x20)  // AIN0 - AIN3
#define ADS112C04_REG_CONFIG0_MUX_AIN10DIFF (0x30)  // AIN1 - AIN0
#define ADS112C04_REG_CONFIG0_MUX_AIN12DIFF (0x40)  // AIN1 - AIN2
#define ADS112C04_REG_CONFIG0_MUX_AIN13DIFF (0x50)  // AIN1 - AIN3
#define ADS112C04_REG_CONFIG0_MUX_AIN23DIFF (0x60)  // AIN2 - AIN3
#define ADS112C04_REG_CONFIG0_MUX_AIN32DIFF (0x70)  // AIN3 - AIN2
#define ADS112C04_REG_CONFIG0_MUX_AIN0SE    (0x80)  // AIN0 single-ended
#define ADS112C04_REG_CONFIG0_MUX_AIN1SE    (0x90)  // AIN1 single-ended
#define ADS112C04_REG_CONFIG0_MUX_AIN2SE    (0xA0)  // AIN2 single-ended
#define ADS112C04_REG_CONFIG0_MUX_AIN3SE    (0xB0)  // AIN3 single-ended
#define ADS112C04_REG_CONFIG0_MUX_AREFMON   (0xC0)  // (Vrefp-Vrefn)/4 reference monitor
#define ADS112C04_REG_CONFIG0_MUX_AVMON     (0xD0)  // (AVDD-AVSS)/4 supply monitor
#define ADS112C04_REG_CONFIG0_MUX_ASHORTMID (0xE0)  // AINp and AINn shorted to (AVDD+AVSS)/2
#define ADS112C04_REG_CONFIG0_MUX_RESERVED  (0xF0)  // Reserved

// 3:1 Programmable Gain Amplifier Configuration (PGA)
#define ADS112C04_REG_CONFIG_PGA_MASK \
    (0x07)  // Programmable gain amplifier configuration
#define ADS112C04_REG_CONFIG0_PGA_1     (0x00)  // Gain 1 (default)
#define ADS112C04_REG_CONFIG0_PGA_2     (0x01)  // Gain 2
#define ADS112C04_REG_CONFIG0_PGA_4     (0x02)  // Gain 4
#define ADS112C04_REG_CONFIG0_PGA_8     (0x03)  // Gain 8
#define ADS112C04_REG_CONFIG0_PGA_16    (0x04)  // Gain 16
#define ADS112C04_REG_CONFIG0_PGA_32    (0x05)  // Gain 32
#define ADS112C04_REG_CONFIG0_PGA_64    (0x06)  // Gain 64
#define ADS112C04_REG_CONFIG0_PGA_128   (0x07)  // Gain 128

// bit 0 - PGA_BYPASS 
#define ADS112C04_REG_CONFIG0_PGA_EN    (0x00)  // PGA enabled (default)
#define ADS112C04_REG_CONFIG0_PGA_BYP   (0x01)  // PGA bypassed

//---------------------------------------------------------------------
// ADS112C04 Configuration Register 1
//---------------------------------------------------------------------
// 7:5 Data Rate (DR)
#define ADS112C04_REG_CONFIG1_DR_MASK   (0xE0)  // Data rate
#define ADS112C04_REG_CONFIG1_DR_20SPS   (0x00)  // 20 samples per second 
#define ADS112C04_REG_CONFIG1_DR_45SPS   (0x20)  // 45 samples per second
#define ADS112C04_REG_CONFIG1_DR_90SPS   (0x40)  // 90 samples per second
#define ADS112C04_REG_CONFIG1_DR_175SPS  (0x60)  // 175 samples per second
#define ADS112C04_REG_CONFIG1_DR_330SPS  (0x80)  // 330 samples per second
#define ADS112C04_REG_CONFIG1_DR_600SPS  (0xA0)  // 600 samples per second
#define ADS112C04_REG_CONFIG1_DR_1000SPS (0xC0)  // 1000 samples per second

// 4 Turbo Mode (MODE)
#define ADS112C04_REG_CONFIG1_MODE_MASK     (0x10)  // Turbo mode
#define ADS112C04_REG_CONFIG1_MODE_NORM     (0x00)  // Normal mode (default)
#define ADS112C04_REG_CONFIG1_MODE_TURBO    (0x10)  // Turbo mode

// 3 Conversion Mode (CM)
#define ADS112C04_REG_CONFIG1_CM_MASK       (0x08)  // Device operating mode
#define ADS112C04_REG_CONFIG1_CM_CONTIN     (0x00)  // Continuous conversion mode (default)
#define ADS112C04_REG_CONFIG1_CM_SINGLE     (0x08)  // Single-conversion mode

// 2:1 Voltage Reference (VREF)
#define ADS112C04_REG_CONFIG1_VREF_MASK     (0x06)  // Voltage reference
#define ADS112C04_REG_CONFIG1_VREF_INT      (0x00)  // Internal 2.048V reference (default)
#define ADS112C04_REG_CONFIG1_VREF_EXT      (0x02)  // External reference
#define ADS112C04_REG_CONFIG1_VREF_SUPPLY   (0x04)  // Analog Supply reference
#define ADS112C04_REG_CONFIG1_VREF_SUPPLY2  (0x06)  // Analog Supply reference (same as above)

// 0 Temperature Sensor Mode (TS)
#define ADS112C04_REG_CONFIG1_TS_MASK       (0x01)  // Temperature sensor mode
#define ADS112C04_REG_CONFIG1_TS_DIS        (0x00)  // Temperature sensor disabled (default)
#define ADS112C04_REG_CONFIG1_TS_EN         (0x01)  // Temperature sensor enabled

//---------------------------------------------------------------------
// ADS112C04 Configuration Register 2
//---------------------------------------------------------------------
// 7 Data Reday (DRDY)
#define ADS112C04_REG_CONFIG2_DRDY_MASK     (0x80)  // Data ready field
#define ADS112C04_REG_CONFIG2_DRDY_NOTRDY   (0x00)  // Data not ready 
#define ADS112C04_REG_CONFIG2_DRDY_RDY      (0x80)  // Data ready 
// 6 Data Counter (DCNT)
#define ADS112C04_REG_CONFIG2_DCNT_MASK     (0x40)  // Data counter enable
#define ADS112C04_REG_CONFIG2_DCNT_DIS      (0x00)  // Conversion counter disabled
#define ADS112C04_REG_CONFIG2_DCNT_EN       (0x40)  // Conversion counter enabled
// 5:4 CRC (CRC)
#define ADS112C04_REG_CONFIG2_CRC_MASK      (0x30)  // CRC field
#define ADS112C04_REG_CONFIG2_CRC_DIS       (0x00)  // CRC disabled (default)
#define ADS112C04_REG_CONFIG2_CRC_INV       (0x10)  // Inverted data output enabled
#define ADS112C04_REG_CONFIG2_CRC_EN        (0x20)  // CRC16 enabled
#define ADS112C04_REG_CONFIG2_CRC_RES       (0x30)  // Reserved
// 3 Burnout Current Source (CBS)
#define ADS112C04_REG_CONFIG2_CBS_MASK      (0x08)  // Burnout current source
#define ADS112C04_REG_CONFIG2_CBS_DIS       (0x00)  // Burnout current source disabled
#define ADS112C04_REG_CONFIG2_CBS_EN        (0x08)  // Burnout current source enabled
// 2:0 IDAC Current (IDAC)
#define ADS112C04_REG_CONFIG2_IDAC_MASK     (0x07)  // IDAC current
#define ADS112C04_REG_CONFIG2_IDAC_OFF      (0x00)  // IDAC off
#define ADS112C04_REG_CONFIG2_IDAC_10uA     (0x01)  // IDAC 10uA
#define ADS112C04_REG_CONFIG2_IDAC_50uA     (0x02)  // IDAC 50uA
#define ADS112C04_REG_CONFIG2_IDAC_100uA    (0x03)  // IDAC 100uA
#define ADS112C04_REG_CONFIG2_IDAC_250uA    (0x04)  // IDAC 250uA
#define ADS112C04_REG_CONFIG2_IDAC_500uA    (0x05)  // IDAC 500uA
#define ADS112C04_REG_CONFIG2_IDAC_1000uA   (0x06)  // IDAC 1000uA
#define ADS112C04_REG_CONFIG2_IDAC_1500uA   (0x07)  // IDAC 1500uA

//---------------------------------------------------------------------
// ADS112C04 Configuration Register 3
//---------------------------------------------------------------------
// 7:5 IDAC1 Routing (I1MUX)
#define ADS112C04_REG_CONFIG3_I1MUX_MASK    (0xE0)  // IDAC1 routing
#define ADS112C04_REG_CONFIG3_I1MUX_DIS     (0x00)  // IDAC1 disabled
#define ADS112C04_REG_CONFIG3_I1MUX_AIN0    (0x10)  // IDAC1 AIN0
#define ADS112C04_REG_CONFIG3_I1MUX_AIN1    (0x20)  // IDAC1 AIN1
#define ADS112C04_REG_CONFIG3_I1MUX_AIN2    (0x40)  // IDAC1 AIN2
#define ADS112C04_REG_CONFIG3_I1MUX_AIN3    (0x80)  // IDAC1 AIN3
#define ADS112C04_REG_CONFIG3_I1MUX_REFP    (0xA0)  // IDAC1 REFP
#define ADS112C04_REG_CONFIG3_I1MUX_REFN    (0xC0)  // IDAC1 REFN
#define ADS112C04_REG_CONFIG3_I1MUX_RSVD    (0xE0)  // Reserved
// 4:2 IDAC2 Routing (I2MUX)
#define ADS112C04_REG_CONFIG3_I2MUX_MASK    (0xE0)  // IDAC2 routing
#define ADS112C04_REG_CONFIG3_I2MUX_DIS     (0x00)  // IDAC2 disabled
#define ADS112C04_REG_CONFIG3_I2MUX_AIN0    (0x02)  // IDAC2 AIN0
#define ADS112C04_REG_CONFIG3_I2MUX_AIN1    (0x04)  // IDAC2 AIN1
#define ADS112C04_REG_CONFIG3_I2MUX_AIN2    (0x08)  // IDAC2 AIN2
#define ADS112C04_REG_CONFIG3_I2MUX_AIN3    (0x10)  // IDAC2 AIN3
#define ADS112C04_REG_CONFIG3_I2MUX_REFP    (0x14)  // IDAC2 REFP
#define ADS112C04_REG_CONFIG3_I2MUX_REFN    (0x18)  // IDAC2 REFN
#define ADS112C04_REG_CONFIG3_I2MUX_RSVD    (0x1C)  // Reserved



/**************************************************************************/
typedef enum {
    CONFIG0 = ADS112C04_REG_CONFIG0,
    CONFIG1 = ADS112C04_REG_CONFIG1,
    CONFIG2 = ADS112C04_REG_CONFIG2,
    CONFIG3 = ADS112C04_REG_CONFIG3
} adsConfigReg_t;

typedef enum {
    AIN01DIFF = ADS112C04_REG_CONFIG0_MUX_AIN01DIFF,
    AIN02DIFF = ADS112C04_REG_CONFIG0_MUX_AIN02DIFF,
    AIN03DIFF = ADS112C04_REG_CONFIG0_MUX_AIN03DIFF,
    AIN10DIFF = ADS112C04_REG_CONFIG0_MUX_AIN10DIFF,
    AIN12DIFF = ADS112C04_REG_CONFIG0_MUX_AIN12DIFF,
    AIN13DIFF = ADS112C04_REG_CONFIG0_MUX_AIN13DIFF,
    AIN23DIFF = ADS112C04_REG_CONFIG0_MUX_AIN23DIFF,
    AIN32DIFF = ADS112C04_REG_CONFIG0_MUX_AIN32DIFF,
    AIN0SE    = ADS112C04_REG_CONFIG0_MUX_AIN0SE,
    AIN1SE    = ADS112C04_REG_CONFIG0_MUX_AIN1SE,
    AIN2SE    = ADS112C04_REG_CONFIG0_MUX_AIN2SE,
    AIN3SE    = ADS112C04_REG_CONFIG0_MUX_AIN3SE,
    AREFMON   = ADS112C04_REG_CONFIG0_MUX_AREFMON,
    AVMON     = ADS112C04_REG_CONFIG0_MUX_AVMON,
    ASHORTMID = ADS112C04_REG_CONFIG0_MUX_ASHORTMID
} adsInMux_t;

typedef enum {
    CNVSTAT_BUSY = ADS112C04_REG_CONFIG2_DRDY_NOTRDY,
    CNVSTAT_DRDY = ADS112C04_REG_CONFIG2_DRDY_RDY
} adsCnvStat_t;

typedef enum {
    MODE_CONTIN = ADS112C04_REG_CONFIG1_CM_CONTIN,
    MODE_SINGLE = ADS112C04_REG_CONFIG1_CM_SINGLE
} adsCnvMode_t;

typedef enum {
    RATE_20   = 20,
    RATE_40   = 40,
    RATE_45   = 45,
    RATE_90   = 90,
    RATE_175  = 175,
    RATE_180  = 180,
    RATE_330  = 330,
    RATE_350  = 350,
    RATE_600  = 600,
    RATE_660  = 660,
    RATE_1000 = 1000,
    RATE_1200 = 1200,
    RATE_2000 = 2000,
} adsRate_t;

#define NUMRATES 13
extern adsRate_t rates[];

typedef enum {
    NRATE_20   = ADS112C04_REG_CONFIG1_DR_20SPS,   // non turbo only
    NRATE_45   = ADS112C04_REG_CONFIG1_DR_45SPS,
    NRATE_90   = ADS112C04_REG_CONFIG1_DR_90SPS,     
    NRATE_175  = ADS112C04_REG_CONFIG1_DR_175SPS,  // non turbo only
    NRATE_330  = ADS112C04_REG_CONFIG1_DR_330SPS,
    NRATE_600  = ADS112C04_REG_CONFIG1_DR_600SPS,
    NRATE_1000 = ADS112C04_REG_CONFIG1_DR_1000SPS
} adsNormRate_t;

typedef enum {
    TURBO_NORM  = ADS112C04_REG_CONFIG1_MODE_NORM,
    TURBO_TURBO = ADS112C04_REG_CONFIG1_MODE_TURBO
} adsTurbo_t;

typedef enum {
    GAIN_1  = ADS112C04_REG_CONFIG0_PGA_1,
    GAIN_2  = ADS112C04_REG_CONFIG0_PGA_2,
    GAIN_4  = ADS112C04_REG_CONFIG0_PGA_4,
    GAIN_8  = ADS112C04_REG_CONFIG0_PGA_8,
    GAIN_16 = ADS112C04_REG_CONFIG0_PGA_16,
    GAIN_32 = ADS112C04_REG_CONFIG0_PGA_32,
    GAIN_64 = ADS112C04_REG_CONFIG0_PGA_64,
    GAIN_128= ADS112C04_REG_CONFIG0_PGA_128
} adsGain_t;

typedef enum {
    PGA_EN  = ADS112C04_REG_CONFIG0_PGA_EN,
    PGA_BYP  = ADS112C04_REG_CONFIG0_PGA_BYP
} adsPGAbyp_t;

typedef enum {
    VREF_INT    = ADS112C04_REG_CONFIG1_VREF_INT,
    VREF_EXT    = ADS112C04_REG_CONFIG1_VREF_EXT,
    VREF_SUPPLY = ADS112C04_REG_CONFIG1_VREF_SUPPLY,
    VREF_SUPPLY2= ADS112C04_REG_CONFIG1_VREF_SUPPLY2
} adsVref_t;

class ADS112C04 {
   protected:
    // Instance-specific properties
    adsInMux_t ads_inmux;
    uint16_t ads_conversionDelay;
    adsCnvMode_t ads_cnvmode; // 
    adsNormRate_t ads_normrate;
    adsRate_t ads_rate;
    adsGain_t ads_gain;
    adsPGAbyp_t ads_pgabyp;
    adsTurbo_t ads_turbo;
    adsVref_t ads_vref;

   public:
    uint8_t ads_i2cAddress;
    void setAddr_ADS112C04(uint8_t i2cAddress=ADS112C04_DEFAULT_ADDRESS);
    void begin(void);
    void writeConfig(void);
    int16_t Measure_Differential();
    void setInMux(adsInMux_t mux);
    adsInMux_t getInMux(void);
    void setCnvMode(adsCnvMode_t mode);
    adsCnvMode_t getCnvMode(void);
    void setRate(adsRate_t rate);
    adsRate_t getRate(void);
    int getSampRate(void);
    void setGain(adsGain_t gain);
    adsGain_t getGain(void);
    void setPGAbyp(adsPGAbyp_t pgabyp);
    adsPGAbyp_t getPGAbyp(void);
    void setTurbo(adsTurbo_t turbo);
    adsTurbo_t getTurbo(void);
    void setVref(adsVref_t vref);
    adsVref_t getVref(void);

    static void writeConfReg(uint8_t i2cAddress, adsConfigReg_t reg, uint8_t value);
    static uint8_t readConfReg(uint8_t i2cAddress, adsConfigReg_t reg);
    static void startConv(uint8_t i2cAddress);
    static uint16_t readData(uint8_t i2cAddress);
};