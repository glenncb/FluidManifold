// i2c_acc - i2c access HAL - uses Arduino Wire functions
#include <Arduino.h>
#include "i2c_acc.h"
#include "pins.h"
#include "modbusmap_gen.h"

// constructor
I2C::I2C(void)
{
    curAddr = 0;
    memset(erraddr, 0, sizeof(erraddr));
    memset(badaddr, 0, sizeof(badaddr));
    memset(reqaddr, 0, sizeof(reqaddr));
}

// init the i2c address tracking arrays
void I2C::init(TwoWire *i2cWire)
{
    // set the Wirep pointer
    if(i2cWire != NULL) Wirep = i2cWire;
    else ESP_LOGE("I2C::init", "i2cWire pointer is NULL");
}

// return 0 if we receive Ack from that address, -1 if no response
bool I2C::checkAddr(i2c_address_t address)
{
    bool res = false;

    // check for response from the address
    xSemaphoreTake(i2cInUse, portMAX_DELAY);
    Wirep->beginTransmission(address);
    res = (Wirep->endTransmission() == 0);
    xSemaphoreGive(i2cInUse);

	return res;
}

// set the address pointer to the target slave address, does not automatically check for response
void I2C::setAddr(i2c_address_t address)
{
    curAddr = address;
    return;
}

// read a byte from register selected, return -1 if unsuccessful or 0 padded byte if successful
int I2C::readByteReg(i2c_address_t address, uint8_t reg)
{
    uint8_t data;
    uint8_t val = -1;
    int bytes;

    // read the register
    bytes = readNByteReg(address, reg, &data, 1);
    // if we got a complete read, calculate the value
    val = (bytes == 1) ? data & 0x000000FF : -1;
    // return the value
    return val;
}

// read a word from register selected, return -1 if unsuccessful or 0 padded word if successful
int I2C::readWordReg(i2c_address_t address, uint8_t reg, bool reverse)
{
    uint8_t data[2];
    uint16_t val;
    int bytes;

    // read the register
    bytes = readNByteReg(address, reg, data, 2);
    // if we got a complete read, calculate the value, else return -1
    
    // normally expect first byte is high, second is low but reverse if flag set
    if(reverse)
        val = (bytes == 2) ? ((data[1] << 8) | (data[0])) & 0x0000FFFF : -1;
    else
        val = (bytes == 2) ? ((data[0] << 8) | (data[1])) & 0x0000FFFF : -1;

    // return the value
    return val;
}

int I2C::readNByteReg(i2c_address_t address, uint8_t reg, uint8_t *data, int len)
{
    int status = 0;
    xSemaphoreTake(i2cInUse, portMAX_DELAY);

    curAddr = address;

    // check for valid data pointer
    if(data == NULL)
    {
        ESP_LOGE("I2C::readNByteReg", "data buffer is NULL");
        xSemaphoreGive(i2cInUse);
        return -1;
    }

    Wirep->beginTransmission(address); // select the slave
    Wirep->write(reg); // write the target register id to the slave
    status = Wirep->endTransmission(false); 
    //delay(1);
    Wirep->requestFrom(address, len); // request the number of bytes from the slave
    //delay(1);
    if(Wirep->available())
    {
        for (uint8_t i = 0; i < len; ++i) // read the bytes into the data buffer
        {
            int val = Wirep->read();
            if(val == -1) // check for an error
            {
                ESP_LOGE("I2C::readNByteReg", "read error device %0x reg %0x at byte %d", address, reg, i);
                xSemaphoreGive(i2cInUse);
                return i==0 ? -1 : i; // return number of bytes read or -1 if no data read
            }
            data[i] = val;
        }
        // success
        xSemaphoreGive(i2cInUse);
        return len;
    }
    else
    {
        ESP_LOGE("I2C::readNByteReg", "no data available from device %0x reg %0x", address, reg);
        xSemaphoreGive(i2cInUse);
        return -1;
    }
}

// read N bytes of data from the I2C slave device (note there is no target register - needed for devices which directly stream when addressed)
int I2C::readNBytes(i2c_address_t address, uint8_t *data, int len)
{
    xSemaphoreTake(i2cInUse, portMAX_DELAY);
    curAddr = address;

    // check for valid data pointer
    if(data == NULL)
    {
        ESP_LOGE("I2C::readNBytes", "data buffer is NULL");
        xSemaphoreGive(i2cInUse);
        return -1;
    }

    Wirep->beginTransmission(address); // select the slave
    //Wirep->endTransmission(false); // FIXME - should we add error checking here?
    Wirep->endTransmission(true); // use stop at end of transmission to trigger some devices to start streaming data (MCP4725 for example)
    Wirep->requestFrom(address, len); // request the number of bytes from the slave
    for (uint8_t i = 0; i < len; ++i) // read the bytes into the data buffer
    {
        int val = Wirep->read();
        if(val == -1) // check for an error
        {
            ESP_LOGE("I2C::readNBytes", "read error device %0x at byte %d", address, i);
            xSemaphoreGive(i2cInUse);
            return i==0 ? -1 : i; // return number of bytes read or -1 if no data read
        }
        data[i] = val;
    }
    // success
    xSemaphoreGive(i2cInUse);
    return len;
}

// write a single byte of data to the target register within the I2C slave device
int I2C::writeByteReg(i2c_address_t address, uint8_t reg, uint8_t data)
{
    int res;
    res = writeNByteReg(address, reg, &data, 1);
    return (res==1) ? 0 : -1;
}

// write two bytes of data to the target register within the I2C slave device
int I2C::writeWordReg(i2c_address_t address, uint8_t reg, uint16_t data, bool reverse)
{
    uint8_t dat[2];
    int res;
    if(reverse)
    {
        dat[0] = data & 0xFF;
        dat[1] = (data >> 8) & 0xFF;
    }
    else
    {
        dat[0] = (data >> 8) & 0xFF;
        dat[1] = data & 0xFF;
    }
    res = writeNByteReg(address, reg, dat, 2);
    return (res==2) ? 0 : -1;
}

// write N bytes of data to the target register within the I2C slave device
int I2C::writeNByteReg(i2c_address_t address, uint8_t reg, uint8_t* data, int len)
{
    xSemaphoreTake(i2cInUse, portMAX_DELAY);
    curAddr = address;

    // check for valid data pointer
    if(data == NULL)
    {
        ESP_LOGE("I2C::writeNByteReg", "data buffer is NULL");
        xSemaphoreGive(i2cInUse);
        return -1;
    }
    Wirep->beginTransmission(address);
    Wirep->write(reg);
    for (uint8_t i = 0; i < len; i++)
    {
        if(Wirep->write(data[i]) != 1)
        {
            ESP_LOGE("I2C::writeNByteReg", "write error device %0x reg %d at byte %d", address, reg, i);
            len = i==0 ? -1 : i; // return number of bytes written or -1 if no data written
            break;
        }
    }
    Wirep->endTransmission();
    xSemaphoreGive(i2cInUse);
    return len;
}

// write single byte without a command reg
int I2C::writeByte(i2c_address_t address, uint8_t data)
{
    int res;

    // call writeNBytes with a single byte
    res = writeNBytes(address, &data, 1);

    return (res=1) ? 0 : -1;
}

// write 2 bytes without a command reg
int I2C::writeWord(i2c_address_t address, uint16_t val, bool reverse)
{
    uint8_t data[2];
    int res;

    if(reverse)
    {
        data[0] = val & 0xFF;		 // send LSByte first
        data[1] = (val >> 8) & 0xFF; // then MSByte
    }
    else
    {
        data[0] = (val >> 8) & 0xFF; // send MSByte first
        data[1] = val & 0xFF;		 // then LSByte
    }

    // write 2 bytes
    res = writeNBytes(address, data, 2);

    return (res==2) ? 0 : -1;
}

// write N bytes of data to the I2C slave device (note there is no target register - needed for devices which directly stream when addressed)
int I2C::writeNBytes(i2c_address_t address, uint8_t* data, int len)
{
    xSemaphoreTake(i2cInUse, portMAX_DELAY);
    curAddr = address;
    // check for valid data pointer
    if(data == NULL)
    {
        ESP_LOGE("I2C::writeNBytes", "data buffer is NULL");
        xSemaphoreGive(i2cInUse);
        return -1;
    }
    Wirep->beginTransmission(address);
    for (uint8_t i = 0; i < len; i++)
    {
        if(Wirep->write(data[i]) != 1)
        {
            ESP_LOGE("I2C::writeNByteReg", "write error device %0x at byte %d", address, i);
            len = i==0 ? -1 : i; // return number of bytes written or -1 if no data written
            break;
        }
    }
    Wirep->endTransmission();
    xSemaphoreGive(i2cInUse);

    return len;
}

// scan I2C addresses for devices and create report similar to i2cdetect
int I2C::scan(bool printmap)
{
    uint8_t res, addr;
    int i, j;
    int nDevices = 0;

    // print the header
    ESP_LOGI("I2C::scan", "Starting scan...");

    if(printmap)
    {
        Serial.printf("I2C device scan:\n");
        Serial.printf("-----------------------------------------------------\n");
        Serial.printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    }

    // loop through the address space in blocks of 16 addresses
    for (i = 0; i < 128; i += 16) 
    {
        if(printmap) Serial.printf("%02x: ", i);
		for(j = 0; j < 16; j++) 
        {
            addr = i + j;
			if(printmap) Serial.flush();
            // The i2c_scanner uses the return value of
            // the Write.endTransmisstion to see if
            // a device did acknowledge to the address.
            Wirep->beginTransmission(addr);
            res = Wirep->endTransmission();
            if (res != 0)
            {
				if(printmap) Serial.printf("-- ");
            }
			else
            {
				if(printmap) Serial.printf("%02x ", addr);
                nDevices++;
            }
		}
		if(printmap) Serial.printf("\n");
    }
    if(printmap)
    { 
        Serial.printf("-----------------------------------------------------\n");

        if(nDevices == 0)
            Serial.printf("No I2C devices found.\n");
        else
            Serial.printf("Found %d I2C devices.\n", nDevices);

        Serial.flush();
    }

    ESP_LOGI("I2C::scan", "Found %d devices", nDevices);
    return nDevices;
}

// instantiate the I2C object
I2C i2c;
// Sempahore for i2c accesses
SemaphoreHandle_t i2cInUse;

// initialize the I2C object and return the number of i2c slaves attached - called from setup()
int I2C_init(void)
{
    int ndevices = 0;
    // initialize the Wire interface
    if(!Wire.begin(PIN_SDA, PIN_SCL, I2C_FREQUENCY))
    {
        ESP_LOGE("I2C_init", "Wire.begin failed");
        return -1;
    }

    // set the I2C timeout to prevent lockups
    Wire.setTimeout(I2C_TIMEOUT);

    // initialize the I2C object
    i2c.init(&Wire);
    // scan the i2c bus
    ndevices = i2c.scan(true);
    // create the semaphore for i2c access
    i2cInUse = xSemaphoreCreateBinary();
    xSemaphoreGive(i2cInUse);
    //dac.Init();         // initialize the DAC object (really I2C now)
    //modDacMinMax();     // set ADC min/max values`
    return ndevices;
}