#pragma once
#include <Arduino.h>
#include "modbus.h"
/*
 * modbus.h - header for modbus access routines
 * Routines to provide access to local modbus resources via the Hreg/Ireg/Coil/Cont functions
 * Resource accesses via these methods operate the same as though they were performed via modbus
 *
 *  Created on: Sat Mar  1 10:43:22 MST 2025
 *  Modified: Sat Mar  1 10:43:22 MST 2025
 *  Author: GlennCB (glenncb@p3pd.com)
 */

// modbus PDU offset: 0=0 based (actual address is same), 1=1 based (actual address = <addr>-1)
#define MB_PDU_OFFSET 0

// process the user specified modbus maps and create the corresponding ModbusRTU registers
#if USE_MODBUS_RTU
extern ModbusRTU MB;
#else
extern ModbusIP MB;
#endif

// prototypes
void modbusInit(void);
int modbusConnect(uint8_t slvid);
void modbusClose(void);
bool modbusReadCoil(int addr);
bool modbusWriteCoil(int addr, bool coildat);
void modbusReadCoils(int addr, int num, bool *coildat);
bool modbusWriteCoils(int addr, int num, bool *coildat);
bool modbusReadContact(int addr);
void modbusReadContacts(int addr, int num, bool *contdat);
uint16_t modbusReadHoldReg(int addr);
void modbusReadHoldRegs(int addr, int num, uint16_t *regdat);
bool modbusWriteHoldReg(int addr, uint16_t regdat);
bool modbusWriteHoldRegs(int addr, int num, uint16_t *regdat);
uint16_t modbusReadInReg(int addr);
void modbusReadInRegs(int addr, int num, uint16_t *regdat);
float modbusRegToFloat(uint16_t *regdat);
void modbusFloatToReg(float fval, uint16_t *regdat);
uint32_t modbusRegToLong(uint16_t *regdat);
void modbusLongToReg(uint32_t lval, uint16_t *regdat);