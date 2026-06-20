#pragma once
// I2C access routines
#include <Wire.h>
#include <stdint.h>

#define I2C_MAX_ADDR 0x7F
// run I2C at 400KHz
#define I2C_FREQUENCY 100000

// I2C timeout limit
#define I2C_TIMEOUT 50

// to probe an address use a read (rather than default of write)
// note that this was needed for 2023.1 and later builds as the kernel smbus driver appears to behave differently
#define I2C_PROBE_READ 1

// attempt to recover I2C bus automatically if any required address reaches this number of errors
#define I2C_AUTORECOVER_THRESHOLD 16
// stop printing error messages once you hit the WARN threshold
#define I2C_WARN_THRESHOLD 4

typedef uint8_t i2c_address_t;
// semaphore for controlling background I2C access 
extern SemaphoreHandle_t i2cInUse;

class I2C
{
  private:
    uint8_t curAddr; // current slave address in use
    uint8_t erraddr[I2C_MAX_ADDR+1]; // number of errors seen at a given address
    bool badaddr[I2C_MAX_ADDR+1]; // map of unresponsive addresses
    bool reqaddr[I2C_MAX_ADDR+1]; // map of required addresses
  public:
    TwoWire *Wirep; // pointer to the TwoWire object
    I2C(void);  // constructor
    /*  @brief Initialize the I2C controller device and map to the associated TwoWire object
     *  @param i2cWire - pointer to the TwoWire object to use for I2C communication
     */
    void init(TwoWire *i2cWire);
    /* @brief check if an I2C address is valid and slave responds at that address
     * @param addr - I2C address to check
     * @return true if the slave responds, false if failed
     */
    bool checkAddr(i2c_address_t addr);
    bool checkAddr(void) { return checkAddr(curAddr); };
    /*  @brief set the current active I2C address
     *  @param addr - taget I2C slave address
     */
    void setAddr(i2c_address_t addr);
    /* @brief read a single byte of data from the target register within the I2C slave device
     * @param addr - I2C address of slave
     * @param reg - register within the slave to read
     * @return 1 byte of data read from the register or -1 if unsuccessful
     */
    int readByteReg(i2c_address_t addr, uint8_t reg);
    int readByteReg(uint8_t reg) { return readByteReg(curAddr, reg); };
    /* @brief read two bytes of data from the target register within the I2C slave device
     * @param addr - I2C address of slave
     * @param reg - register within the slave to read
     * @return data word read from the register or -1 if unsuccessful
     */
    int readWordReg(i2c_address_t address, uint8_t reg, bool reverse=false);
    int readWordReg(uint8_t reg, bool reverse=false) { return readWordReg(curAddr, reg, reverse); };
    /* @brief read N bytes of data from the target register within the I2C slave device
     * @param addr - I2C address of slave
     * @param reg - register within the slave to read
     * @param data - pointer to the data buffer to store the read data
     * @param len - number of bytes to read
     * @return number of bytes read or -1 if unsuccessful
     */
    int readNByteReg(i2c_address_t address, uint8_t reg, uint8_t *data, int len);
    int readNByteReg(uint8_t reg, uint8_t *data, int len) { return readNByteReg(curAddr, reg, data, len); };
    /* @brief read N bytes of data from the I2C slave device (note there is no target register - needed for devices which directly stream when addressed)
     * @param addr - I2C address of slave
     * @param data - pointer to the data buffer to store the read data
     * @param len - number of bytes to read
     * @return number of bytes read or -1 if unsuccessful
     */
    int readNBytes(i2c_address_t address, uint8_t *data, int len);
    int readNBytes(uint8_t *data, int len) { return readNBytes(curAddr, data, len); };
    /* @brief write a single byte of data to the target register within the I2C slave device
     * @param addr - I2C address of slave
     * @param reg - register within the slave to write
     * @param data - data byte to write
     * @return 0 if successful, -1 if failed
     */
    int writeByteReg(i2c_address_t address, uint8_t reg, uint8_t data);
    int writeByteReg(uint8_t reg, uint8_t data) { return writeByteReg(curAddr, reg, data); };
    /* @brief write two bytes of data to the target register within the I2C slave device
     * @param addr - I2C address of slave
     * @param reg - register within the slave to write
     * @param data - data word to write
     * @return 0 if successful, -1 if failed
     */
    int writeWordReg(i2c_address_t address, uint8_t reg, uint16_t data, bool reverse=false);
    int writeWordReg(uint8_t reg, uint16_t data, bool reverse=false) { return writeWordReg(curAddr, reg, data, reverse); };
    /* @brief write N bytes of data to the target register within the I2C slave device
     * @param addr - I2C address of slave
     * @param reg - register within the slave to write
     * @param data - pointer to the data buffer to write
     * @param len - number of bytes to write
     * @return #bytes written if successful, -1 if failed
     */
    int writeNByteReg(i2c_address_t address, uint8_t reg, uint8_t *data, int len);
    int writeNByteReg(uint8_t reg, uint8_t *data, int len) { return writeNByteReg(curAddr, reg, data, len); };
    /* @brief write a single byte of data to slave (no target register)
     * @param addr - I2C address of slave
     * @param data - data byte to write
     * @return 0 if successful, -1 if failed
     */
    int writeByte(i2c_address_t address, uint8_t data);
    int writeByte(uint8_t data) { return writeByte(curAddr, data); };
    /* @brief write two bytes (word) of data to the slave (no target register)
     * @param addr - I2C address of slave
     * @param val - data word to write
     * @return 0 if successful, -1 if failed
     */
    int writeWord(i2c_address_t address, uint16_t val, bool reverse=false);
    int writeWord(uint16_t val, bool reverse=false) { return writeWord(curAddr, val, reverse); };
    /* @brief write N bytes of data to the I2C slave device (note there is no target register - needed for devices which directly stream when addressed)
     * @param addr - I2C address of slave
     * @param data - pointer to the data buffer to write
     * @param len - number of bytes to write
     * @return #bytes written if successful, -1 if failed
     */
    int writeNBytes(i2c_address_t address, uint8_t *data, int len);
    int writeNBytes(uint8_t *data, int len) { return writeNBytes(curAddr, data, len); };
    /* @brief scan the i2c address space and print all responding devices similar to i2cdetect 
     * @param printmap - if true, print the map of responding devices
     */
    int scan(bool printmap);
};


// initialize the I2C object - called from setup()
int I2C_init(void);

// declare the I2C object
extern I2C i2c;
// declare the semaphore for i2c access
extern SemaphoreHandle_t i2cInUse;