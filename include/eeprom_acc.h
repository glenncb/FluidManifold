// defines for EEPROM access routines for reading/writing serial number
#pragma once
#include <stdint.h>

// number of bytes of EEPROM to reserve
#define EE_SERIAL_SIZE 4
#define EEPROM_SIZE (EE_SERIAL_SIZE)

// EEPROM address map
#define EE_SERIAL_ADDR 0

// preferences namespace for the PFRx app
#define PREF_NAMESPACE "INCUBATOR"
// serialnum key for Preferences lib
#define PREF_SERIAL_KEY "Serial"
// WiFi ssid/passwd keys for Preferences lib
#define PREF_WIFISSID_KEY "WifiSSID"
#define PREF_WIFIPWD_KEY "WifiPWD"
// keys used to recover in case of crash or power failure during transport
// number of retries we have attempted, don't bother to retry if we exceed the max retry count
#define PREF_REC_RETRIES "RecRetries"
// Recovery enabled - set to true if we're in locked transport mode and we want to enable auto-recovery on WDT timeout
#define PREF_REC_ENABLED "RecEnabled"
// current target temperature
#define PREF_REC_TEMPTARGET "TempTarget"
// current TEC setpoint
#define PREF_REC_TECSET "TempTecSet"

#define PREF_WIFIAP_KEY "WifiAP"
#define PREF_WIFIEN_KEY "WifiEn"


void eepromInit(bool clear=false);
uint16_t readSerial(void);
void writeSerial(uint16_t serialnum);
#if 0
const char *readWifiSsid(void);
void writeWifiSsid(const char *ssid);
const char *readWifiPass(void);
void writeWifiPass(const char *pass);
bool readWifiModeAP(void);
void writeWifiModeAP(bool apmode);
bool readWifiEnable(void);
void writeWifiEnable(bool enable);
#endif