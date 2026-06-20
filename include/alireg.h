#pragma once
#include <Arduino.h>
#include <stdint.h>

#define ALIREG_UART    Serial1
#define ALIREG_BAUD    115200
#define ALIREG_TIMEOUT 1000

#define ALIREG_NUM 1

typedef enum {
    ALIREG_MONPRESS = 0,    // actual measured pressure
    ALIREG_SETPRESS,        // pressure setpoint
} aregcmd_t;

typedef enum {
    AREG_OFFLINE = 0,
    AREG_ONLINE,
} aregtype_t;

typedef struct {
    float press;            // actual pressure in psi
    float setpoint;         // pressure setpoint in psi
    unsigned long lastread;
} aregdata_t;


class ALIREG {
    private:
        uint8_t addr;
        aregtype_t aregtype;
        aregdata_t aregdata;
        bool ReadDeviceInfo(void);
        bool ReadDeviceData(void);

    public:
        ALIREG(void);
        ~ALIREG(void);

        void Init(uint8_t ad);

        void SetAddr(uint8_t ad) { addr = ad; };
        uint8_t GetAddr(void) { return addr; };
        char AddrCh(void) { return addr + 'A'; };

        float GetPress(void)    { ReadDeviceData(); return aregdata.press; };
        float GetSetpoint(void) { ReadDeviceData(); return aregdata.setpoint; };
        void  SetPressure(float sp);

        bool Online(void) { return (aregtype != AREG_OFFLINE); };
};

void modr_ALIREG(uint16_t *val, uint32_t drvaddr, uint8_t drvchan);
void modw_ALIREG(uint16_t *val, uint32_t drvaddr, uint8_t drvchan);
void initAliReg(void);

extern ALIREG areg[];
extern SemaphoreHandle_t alireg485InUse;
