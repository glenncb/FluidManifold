/*
 * modbus.c - routines to access resources over modbus using libmodbus
 *
 *  Created on: Thu 13 Jun 2024 01:04:35 PM MDT
 *  Modified: Thu 13 Jun 2024 01:04:42 PM MDT
 *  Author: GlennCB (glenncb@p3pd.com)
 */

#include <Arduino.h>
#include "modbus_acc.h"
#include "modbusmap.h"

/**
 * @brief Read a coil
 * @param addr - address of the coil to read
 * @return true if the coil is set, false if not
 */
bool modbusReadCoil(int addr)
{
	// perform a single coil read
	return MB.Coil(addr);
}

/**
 * @brief Write a coil
 * @param addr - address of the coil to write
 * @param coildat - value to write to the coil
 * @return true if coil written successfully, false if not
 */
bool modbusWriteCoil(int addr, bool coildat)
{
	bool ret;

	// write the coil data
	ret = MB.Coil(addr, coildat);
	if(!ret){
		ESP_LOGE("modbusWriteCoil", "MB.Coil(%d, %d): invalid coil", addr, coildat);
	}
	return ret;
}

/**
 * @brief Read <ncoils> consecutive coils starting at address <addr>
 * @param addr - starting address of the coils to read
 * @param ncoils - number of coils to read
 * @param coildat - bool array to store the coil data
 */
void modbusReadCoils(int addr, int ncoils, bool *coildat)
{
	int i;

	// read the coil data
	for(i=0; i<ncoils; i++)
	{
		coildat[i] = MB.Coil(addr+i);
	}	
}

/**
 * @brief Write <ncoils> consecutive coils starting at <addr>, using data from coildat bool array
 * @param addr - starting address of the coils to write
 * @param ncoils - number of coils to write
 * @param coildat - byte array containing the coil data
 * @return true if coils written successfully, false if not
 */
bool modbusWriteCoils(int addr, int ncoils, bool *coildat)
{
	int ret;
	int i;

	// write the coil data
	for(i=0;i<ncoils;i++)
	{
		ret = MB.Coil(addr+i, coildat[i]);
		if(!ret){
			ESP_LOGE("modbusWriteCoils", "MB.Coil(%d, %d): invalid coil", addr+i, coildat[i]);
			return false;
		}
	}
	return true;
}

/**
 * @brief Read a single contact
 * @param addr - address of the contact to read
 * @return true if the contact is set, false if not
 */
bool modbusReadContact(int addr)
{
	return MB.Ists(addr);
}

// read <ncont> consecutive contacts starting at <addr>, store return data in contdat byte array
/**
 * @brief read <ncont> consecutive contacts starting at <addr>, store return data in contdat byte array
 * @param addr - starting address of the contacts to read
 * @param ncont - number of contacts to read
 * @param contdat - byte array to store the contact data
 */
void modbusReadContacts(int addr, int ncont, uint8_t *contdat)
{
	int i;

	// read the coil data
	for(i=0; i<ncont; i++)
	{
		contdat[i] = MB.Ists(addr+i);
	}	
}

/**
 * @brief read a single holding register
 * @param addr - address of the holding register to read
 * @return value of the holding register
 */
uint16_t modbusReadHoldReg(int addr)
{
	return MB.Hreg(addr);
}

/**
 * @brief read multiple <nregs> holding registers starting at <addr>, store return data in regdat word array
 * @param addr - starting address of the holding registers to read
 * @param nregs - number of holding registers to read
 * @param regdat - word array to store the register data
 */
void modbusReadHoldRegs(int addr, int nregs, uint16_t *regdat)
{
	int i;

	// read the coil data
	for(i=0;i<nregs;i++)
	{
		regdat[i] = MB.Hreg(addr+i);
	}
}

// write single holding register
/**
 * @brief write a single holding register
 * @param addr - address of the holding register to write
 * @param regdat - value to write to the holding register
 * @return true if register written successfully, false if not
 */
bool modbusWriteHoldReg(int addr, uint16_t regdat)
{
	return modbusWriteHoldRegs(addr, 1, &regdat);
}

// write <nregs> holding registers starting at <addr>, using data from regdat word array
/**
 * @brief write <nregs> holding registers starting at <addr>, using data from regdat word array
 * @param addr - starting address of the holding registers to write
 * @param nregs - number of holding registers to write
 * @param regdat - word array containing the register data
 * @return true if registers written successfully, false if not
 */
bool modbusWriteHoldRegs(int addr, int nregs, uint16_t *regdat)
{
	int ret;
	int i;

	// write the register data
	for(i=0; i<nregs; i++)
	{
		ret = MB.Hreg(addr+i, regdat[i]);
		if(!ret){
			ESP_LOGE("modbusWriteHoldRegs", "MB.Hreg(%d, %d): invalid register", addr+i, regdat[i]);
			return false;
		}
	}

	return ret;
}

/**
 * @brief read a single input register
 * @param addr - address of the input register to read
 * @return value of the input register
 */
uint16_t modbusReadInReg(int addr)
{
	return MB.Ireg(addr);
}

// read multiple <nregs> input registers starting at <addr>, store return data in regdat word array
void modbusReadInRegs(int addr, int nregs, uint16_t *regdat)
{
	int i;

	for(i=0; i<nregs; i++)
	{
		regdat[i] = MB.Ireg(addr+i);
	}
}

// convert a pair of 16b register data to float
float modbusRegToFloat(uint16_t *regdat)
{
	union {
		uint32_t lval;
		float fval;
	} dat;

	dat.lval = (regdat[1]<<16) | regdat[0];

	//fval = modbus_get_float_dcba(regdat);

	return dat.fval;
}

// convert a float to a pair of 16b register data
void modbusFloatToReg(float fval, uint16_t *regdat)
{
	union {
		uint16_t val[2];
		float fval;
	} dat;
	//modbus_set_float_dcba(fval, regdat);
	dat.fval = fval;
	regdat[0] = dat.val[0];
	regdat[1] = dat.val[1];
}

// convert a pair of 16b register data to 32b unsigned int
uint32_t modbusRegToLong(uint16_t *regdat)
{
	long int lval;

	lval = (regdat[0]<<16) | regdat[1];

	return lval;
}

// convert a 32b unsigned int to a pair of 16b register data
void modbusLongToReg(uint32_t lval, uint16_t *regdat)
{
	regdat[0] = (lval>>16) & 0xFFFF;
	regdat[1] = lval & 0xFFFF;
}
