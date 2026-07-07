// modbusota.h - OTA firmware updates over modbus for the FluidManifold driver-dispatch map
// Ported from the DropDelay project (ESP32/DropDelayESP32/Common/modbusota.*) and adapted to
// FluidManifold's resdriver_t / mod_drvrGet/Set architecture.
// based on the example from https://medium.com/@andrirahmadhani/device-firmware-update-dfu-over-modbus-rtu-protocol-modbus-slave-esp32-cf9c8065ae76
#pragma once
#include <stdint.h>
#include "modbusmap.h"      // wordresource_t

// FWREGS registers = FWBUFSZ bytes per FWDATA burst.  FWREGS must be < 123 (modbus FC16 limit).
#define FWREGS      100
#define FWBUFSZ     (FWREGS*2)   // 200 bytes
#define FWDATAREG   200          // base modbus holding-register number of the FWDATA block
#define FWBOGUSREG  0xFFFF       // sentinel: no pending multi-register write

// initialization routine for OTA (call once after modbusRegSetup)
void modbusOTASetup(void);

// -----------------------------------------------------------------------------
// Driver wrappers dispatched from the switch() blocks in modbusmap.cpp.
// One wrapper per OTA register, matching the per-register DRVR_* enum convention.
// -----------------------------------------------------------------------------
// coils (bit resources) - read returns current state, write drives the FSM
bool modr_FWUPDSTART(void);     void modw_FWUPDSTART(bool val);
bool modr_FWUPDEND(void);       void modw_FWUPDEND(bool val);
bool modr_FWUPDABORT(void);     void modw_FWUPDABORT(bool val);

// 32-bit holding registers (2 regs each) - fill/consume the shared regvals[] array
// regvals[0] = low 16 bits (base register), regvals[1] = high 16 bits
void modr_FWSIZE(uint16_t *regvals);        void modw_FWSIZE(uint16_t *regvals);
void modr_FWCHECKSUM(uint16_t *regvals);    void modw_FWCHECKSUM(uint16_t *regvals);

// FWDATA multi-register block - bypasses the MAXDRVREGS-sized regvals[] buffer and accumulates
// directly into fwbuf, flushing a full burst to flash via ota.write()
uint16_t modw_FWDATAblock(uint16_t val, uint16_t regaddr, const wordresource_t *rp);
uint16_t modr_FWDATAword(uint16_t regaddr, const wordresource_t *rp);   // readback (non-critical)

// input registers (read-only)
uint16_t modr_FWUPDSTATE(void);
uint16_t modr_FWUPDERR(void);
uint16_t modr_FWUPDPERCENT(void);

// firmware update accumulation buffer
extern uint8_t fwbuf[];
