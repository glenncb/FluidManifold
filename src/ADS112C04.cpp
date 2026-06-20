// ADS112C04 library modified from M5Stack to use i2c_acc library for Wire access
#include "ADS112C04.h"
#include "i2c_acc.h"
#include "esp_log.h"


// array for checking valid sample rates
adsRate_t rates[] = {RATE_20, RATE_40, RATE_45, RATE_90, RATE_175, RATE_180, RATE_330, RATE_350, RATE_600, RATE_660, RATE_1000, RATE_1200, RATE_2000};

/*! @brief Sets up the Hardware.*/
void ADS112C04::begin() {
    //Wire.begin();
}

/*! @brief Instantiates a new ADS112C04 class with appropriate properties.*/
void ADS112C04::setAddr_ADS112C04(uint8_t i2cAddress) {
    ads_i2cAddress      = i2cAddress;
    // set defaults
    ads_conversionDelay = ADS112C04_CONVERSIONDELAY;
    ads_turbo = TURBO_NORM;
    ads_cnvmode = MODE_SINGLE;
    ads_normrate = NRATE_20;
    ads_pgabyp = PGA_EN;
    ads_gain = GAIN_2;
    ads_inmux = AIN0SE;
}

/*! @brief Writes 8-bits to the destination register.*/
void ADS112C04::writeConfReg(uint8_t i2cAddress, adsConfigReg_t reg, uint8_t value) {
    uint8_t cmd = ADS112C04_CMD_WREG;
    int resp;
    cmd |= reg;
    resp = i2c.writeByteReg(i2cAddress, cmd, value);
    if(resp < 0)
    {
        ESP_LOGE("ADS112C04::writeConfReg", "i2c write addr %02x, reg %02x failed", i2cAddress, cmd);
    }
    //Wire.beginTransmission(i2cAddress);
    //i2cwrite(cmd);
    //i2cwrite((uint8_t)value);
    //Wire.endTransmission();
}

/*! @brief Reads 16-bits from the destination register.*/
uint8_t ADS112C04::readConfReg(uint8_t i2cAddress, adsConfigReg_t reg) {
    uint8_t cmd = ADS112C04_CMD_RREG;
    int resp;
    cmd |= reg;
    //Wire.beginTransmission(i2cAddress);
    //i2cwrite(cmd);
    //Wire.endTransmission();
    //Wire.requestFrom(i2cAddress, (uint8_t)1);
    resp = i2c.readByteReg(i2cAddress, cmd);
    if(resp < 0)
    {
        ESP_LOGE("ADS112C04::readConfReg", "i2c read addr %02x, reg %02x failed", i2cAddress, cmd);
    }
    //return (uint8_t)(i2cread());
    return (resp & 0x0FF);
}

/*! @brief Reads 16-bits from the destination register.*/
uint16_t ADS112C04::readData(uint8_t i2cAddress) {
    uint8_t cmd = ADS112C04_CMD_RDATA;
    int resp;

    resp = i2c.readWordReg(i2cAddress, cmd);
    if(resp < 0)
    {
        ESP_LOGE("ADS112C04::readData", "i2c read addr %02x, reg %02x failed", i2cAddress, cmd);
    }

    //Wire.beginTransmission(i2cAddress);
    //i2cwrite(cmd);
    //Wire.endTransmission();
    //Wire.requestFrom(i2cAddress, (uint8_t)2);
    //return (uint16_t)((i2cread() << 8) | i2cread());
    return (resp & 0x0FFFF);
}

void ADS112C04::startConv(uint8_t i2cAddress) {
    uint8_t cmd = ADS112C04_CMD_START;
    //Wire.beginTransmission(i2cAddress);
    //i2cwrite(cmd);
    //Wire.endTransmission();
    i2c.writeByte(i2cAddress, cmd);
}

void ADS112C04::setInMux(adsInMux_t mux) {
    ads_inmux = mux;
}

adsInMux_t ADS112C04::getInMux(void) {
    return ads_inmux;
}

/*! @brief Sets the Operational status/single-shot conversion start
        This determines the operational status of the device.*/
void ADS112C04::setCnvMode(adsCnvMode_t cnvmode) {
    ads_cnvmode = cnvmode;
}

/*! @brief Gets the Operational status/single-shot conversion start.*/
adsCnvMode_t ADS112C04::getCnvMode() {
    return ads_cnvmode;
}

void ADS112C04::setTurbo(adsTurbo_t turbo) {
    ads_turbo = turbo;
}

adsTurbo_t ADS112C04::getTurbo(void) {
    return ads_turbo;
}

void ADS112C04::setVref(adsVref_t vref) {
    ads_vref = vref;
}

adsVref_t ADS112C04::getVref(void) {
    return ads_vref;
}

/*! @brief Sets the Date Rate.
    This controls the data rate setting.*/
void ADS112C04::setRate(adsRate_t rate) {
    bool inturbo = ads_turbo ? 1 : 0;
    ads_rate = rate;
    switch (rate)
    {
        case RATE_20:
            ads_turbo = TURBO_NORM; // 20sps only in normal mode
            ads_normrate = NRATE_20;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY;
            break;
        case RATE_40:
            ads_turbo = TURBO_TURBO; // 40SPS only in turbo mode
            ads_normrate = NRATE_20;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/2;
            break;
        case RATE_45:
            ads_turbo = TURBO_NORM; // 45sps only in normal mode
            ads_normrate = NRATE_45;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/2;
            break;
        case RATE_90:
            ads_normrate = inturbo ? NRATE_45 : NRATE_90;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/4;
            break;
        case RATE_175:
            ads_turbo = TURBO_NORM; // 175sps only in normal mode
            ads_normrate = NRATE_175;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/8;
            break;
        case RATE_180:
            ads_turbo = TURBO_TURBO; // 180sps only in turbo mode
            ads_normrate = NRATE_90;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/8;
            break;
        case RATE_330:
            ads_turbo = TURBO_NORM; // 330sps only in normal mode
            ads_normrate = NRATE_330;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/16;
            break;
        case RATE_350:
            ads_turbo = TURBO_TURBO; // 350sps only in turbo mode
            ads_normrate = NRATE_175;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/16;
            break;
        case RATE_600:
            ads_turbo = TURBO_NORM; // 330sps only in normal mode
            ads_normrate = NRATE_600;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/32 < 2 ? 2 : ADS112C04_CONVERSIONDELAY/32;
            break;
        case RATE_660:
            ads_turbo = TURBO_TURBO; // 660sps only in turbo mode
            ads_normrate = NRATE_330;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/16;
            break;
        case RATE_1000:
            ads_turbo = TURBO_NORM; // 1000sps only in normal mode
            ads_normrate = NRATE_1000;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/64 < 1 ? 1 : ADS112C04_CONVERSIONDELAY/64;
            break;
        case RATE_1200:
            ads_turbo = TURBO_TURBO; // 1200sps only in turbo mode
            ads_normrate = NRATE_600;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/64 < 1 ? 1 : ADS112C04_CONVERSIONDELAY/64;
            break;
        case RATE_2000:
            ads_turbo = TURBO_TURBO; // 2000sps only in turbo mode
            ads_normrate = NRATE_1000;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY/128 < 1 ? 1 : ADS112C04_CONVERSIONDELAY/128;
            break;
        default:
            // shouldn't reach this case, but set to safe default if reached
            ads_rate = RATE_20;
            ads_turbo = TURBO_NORM;
            ads_normrate = NRATE_20;
            ads_conversionDelay = ADS112C04_CONVERSIONDELAY;
            break;
    }
}

/*! @brief Gets the Date Rate.*/
adsRate_t ADS112C04::getRate() {
    return ads_rate;
}

int ADS112C04::getSampRate() {
    int i;
    for(i=0;i<NUMRATES; i++) {
        if(ads_rate == rates[i]) {
            return rates[i];
        }
    }   
    return -1;
}

/*! @brief Sets the gain and input voltage range
    This configures the programmable gain amplifier.*/
void ADS112C04::setGain(adsGain_t gain) {
    ads_gain = gain;
}

/*! @brief Gets a gain and input voltage range.*/
adsGain_t ADS112C04::getGain() {
    return ads_gain;
}

// load the ADC settings into the config registers
void ADS112C04::writeConfig(void) {
    // Start with default values
    uint16_t config;

    // config0 register - set mux, gain, and PGA bypass
    config = ads_inmux | ads_gain | ads_pgabyp;
    // write CONFIG0
    writeConfReg(ads_i2cAddress, CONFIG0, config);
    // config1 register - data rate, mode, and turbo
    config = ads_normrate | ads_turbo | ads_cnvmode | ads_vref;
    // write CONFIG0
    writeConfReg(ads_i2cAddress, CONFIG1, config);

    // read back config reg
    config = readConfReg(ads_i2cAddress, CONFIG0);
    //Serial.printf("CONFIG0: 0x%02X\n", config);
    config = readConfReg(ads_i2cAddress, CONFIG1);
    //Serial.printf("CONFIG1: 0x%02X\n", config);
}

/*! @brief Reads the conversion results, measuring the voltage
    difference between the P and N input
    Generates a signed value since the difference can be either
    positive or negative.*/
int16_t ADS112C04::Measure_Differential() {
    // Start the conversion
    startConv(ads_i2cAddress);

    // Wait for the conversion to complete
    delay(ads_conversionDelay);

    // Read the conversion results
    uint16_t raw_adc = readData(ads_i2cAddress);
    return (int16_t)raw_adc;
}
