#pragma once
#include "modbusmap.h"

//----------------------------------------------------
// enums for each modbus resource and the mapped modbus resource address (1 based)
// these will be used in the map tables to define the modbus register numbers of each resource
//----------------------------------------------------
// Coils
typedef enum {
    COIL_ERROR=1,
    COIL_RESET=2,
    COIL_STATLED=3,
    COIL_ERRLED=4,
    COIL_BOOST_REG=10,
    COIL_SAMPLE_REG=11,
    COIL_PINCH_FULL=12,
    COIL_PINCH_SOFT=13,
    COIL_NOZZLE1=14,
    COIL_NOZZLE1_SELECT=15,
    COIL_NOZZLE2=16,
    COIL_NOZZLE2_SELECT=17,
    COIL_WASTE_CATCHER=18,
    COIL_DROP_TRAY=19,
    COIL_SAMPLE_STATION=20,
} modcoilnums_t;

// Contacts (discrete inputs)
typedef enum {
    CONT_ERROR=1,
    CONT_BOOTSW=2,
    CONT_VACUUM_SWITCH=10,
    CONT_FANALERT=11,
} modcontnums_t;

// holding regs
typedef enum {
    HOLD_CONFIG=1,
    HOLD_SERIALNUM_PROG=2,
    HOLD_REGBAUD=3,
    HOLD_LEDCOLOR=4,
    HOLD_LEDBRIGHTPCT=6,
    HOLD_FAN_RPM_SET1=10,
    HOLD_FAN_RPM_SET2=11,
    HOLD_CTLMODE=12,
    HOLD_TEMPTARGET=13,
    HOLD_FANPID_INTERVAL=15,
    HOLD_FANPID_KP=16,
    HOLD_FANPID_KI=18,
    HOLD_FANPID_KD=20,
    HOLD_PRESSURE_SET_ANALOG=22,
    HOLD_PRESSURE_SET_DIGITAL=24,
} modholdnums_t;

// input regs
typedef enum {
    INPUT_BOARDID=1,
    INPUT_FWVER=2,
    INPUT_SERIALNUM=3,
    INPUT_STATUSREG=4,
    INPUT_ERRNUM=5,
    INPUT_FAN_RPM_MON1=10,
    INPUT_FAN_RPM_MON2=11,
    INPUT_WIFI_RSSI=12,
    INPUT_BOARD_TEMP=13,
    INPUT_PRESSURE_MON_ANALOG=15,
    INPUT_PRESSURE_MON_DIGITAL=17,
    INPUT_OMEGA_PRESSURE=19,
    INPUT_LASER_TEMP=21,
} modinputnums_t;

extern const bitresource_t coilmap[];
extern const bitresource_t contmap[];
extern const wordresource_t holdregmap[];
extern const wordresource_t inregmap[];
