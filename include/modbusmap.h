// modbusmap.h - maps modbus resources to user supplied variables/callbacks
//----------------------------------------------------
// Do not modify this file - all user code should be in modbusmapuser.cpp / modbusmapuser.h
//----------------------------------------------------
#pragma once
#include <cstdint>
#if USE_MODBUS_RTU
#include <ModbusRTU.h>
#else
#include <ModbusIP_ESP8266.h>
#endif
#include "alireg.h"

// set this to the largest number of registers used by the driver (i.e. 4 for float, 8 for double, etc)
#define MAXDRVREGS 8

typedef enum {
    RES_RO=0,
    RES_RW=1
} resacc_t;

typedef enum {
	DRVR_NONE=0,		// no driver, just utilize builtin modbus support
	DRVR_BITVAR,		// simple bit driver utilizes Addr as the pointer and Chan as the bit number
	DRVR_VAR,			// simple variable driver utilizes Addr as the pointer to the variable and Chan as number of bytes
	DRVR_FLASH,			// flash driver for storing data in flash memory
	DRVR_GPIO,			// GPIO driver
	DRVR_PWM,			// LEDC PWM parameter (freq, duty cycle) holding register driver
	DRVR_PWMEN,			// LEDC PWM enable/disable coil driver
	DRVR_FAN,			// EMC2302 fan controller driver
	DRVR_DAC,			// driver for MCP4725 DAC
	DRVR_ADC,			// SPI driver for ADS112C04 ADC
	DRVR_INTADC,		// internal ADC driver
	DRVR_SPIRTD,		// RTD temp sensor driver
	DRVR_SERLED,		// serial LED driver
	DRVR_TEMP,			// MCP9808 temperature sensor driver
	DRVR_ALIREG,		// Alicat serial RS-485 connected regulator
	// OTA firmware update drivers (see modbusota.cpp) - one per FW* register
	DRVR_FWUPDSTART,	// coil: begin OTA update
	DRVR_FWUPDEND,		// coil: finalize/validate OTA update
	DRVR_FWUPDABORT,	// coil: abort OTA update
	DRVR_FWSIZE,		// holdreg: 32-bit firmware image size in bytes
	DRVR_FWCHECKSUM,	// holdreg: 32-bit firmware checksum (reserved)
	DRVR_FWDATA,		// holdreg: multi-register firmware data block (bypasses regvals[])
	DRVR_FWUPDSTATE,	// inreg: OTA state machine value
	DRVR_FWUPDERR,		// inreg: OTA error code
	DRVR_FWUPDPERCENT,	// inreg: OTA write percent complete
	DRVR_FWUPDBYTES,	// inreg: 32-bit exact count of bytes actually written to flash so far
} resdriver_t;

// structure used to for modbus bit level resources (cois and discrete inputs)
typedef struct {
    int num;        	// resource number
    const char *name;   // name of the resource
	resacc_t acc;		// read/write or read only attribute
	bool init;			// initial value of the resource
	resdriver_t drvr;	// driver type for the resource
	uint32_t drvaddr;	// driver address for the resource (i.e. ADC1 vs. ADC2) or a pointer to a variable
	uint8_t drvchan;		// driver channel for the resource (i.e. which channel of the ADC 0..11)
	//uint16_t (*funcr)(void);	// function to call on read, NULL if none 
	//void (*funcw)(uint16_t);	// function to call on write, NULL if none 
} bitresource_t;

// structure used for modbus word resources (holding and input registers)
typedef struct {
    int num;        	// resource number
    const char *name;   // name of the resource
	resacc_t acc;		// read/write or read only attribute
	uint16_t init;		// initial value of the resource
	char fmt;			// format of the resource (b=bit, i=int, u=unsigned, f=float, s=string)
	uint8_t len;			// # bytes of the resource
	//void *varp;	    	// variable pointer if it is a simple variable, NULL if it is a function
	//int regs;			// # registers of the resource var
	resdriver_t drvr;	// driver type for the resource
	uint32_t drvaddr;	// driver address for the resource (i.e. ADC1 vs. ADC2) or a pointer to a variable
	int8_t drvchan;		// driver channel (negative value = raw read for ADC driver)
	// depracate use of read/write functions and use the drvr instead
	//uint16_t (*funcr)(void);	// function to call on read, NULL if none 
	//void (*funcw)(uint16_t);	// function to call on write, NULL if none 
} wordresource_t;

typedef union {
    bitresource_t bitres;
    wordresource_t wordres;
} resmap_t;


// loads all registers defined in the map tables to ModbusRTU object and adds callbacks for set/get
// process the user specified modbus maps and create the corresponding ModbusRTU registers
#if USE_MODBUS_RTU
void modbusRegSetup(ModbusRTU *mb);
#else
void modbusRegSetup(ModbusIP *mb);
#endif
// debug print of a modbus register
void modPrintReg(const char *prefix, TRegister* reg, const char *name, uint16_t val);
bool mod_drvrSetBit(bool bitval, uint16_t regaddr, const bitresource_t *rp);
bool mod_drvrGetBit(uint16_t regaddr, const bitresource_t *rp);
uint16_t mod_drvrGet(uint16_t regaddr, const wordresource_t *rp, bool ishreg);
uint16_t mod_drvrSet(uint16_t val, uint16_t regaddr, const wordresource_t *rp);

// map tables
extern const bitresource_t coilmap[];
extern const bitresource_t contmap[];
extern const wordresource_t holdregmap[];
extern const wordresource_t inregmap[];

extern int multireg_wr_regnum; // last register number written in a multi-reg write 
