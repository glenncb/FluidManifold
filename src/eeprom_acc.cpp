// EEPROM routines to save/restore persistent data
// Location Data
// 0x00-0x01: serial number
#include <stdint.h>
#include <EEPROM.h>
#include <Preferences.h>
#include "eeprom_acc.h"

// use Preferences library for parameter storage
Preferences prefs;

// initialize the EEPROM
void eepromInit(bool clear) 
{
  if(EEPROM.begin(EEPROM_SIZE) == false)
  {
    Serial.println("EEPROM initialization failed.");
  }

  // also initialize the preferences section in r/w mode (to store SSID and process info)
  if(prefs.begin(PREF_NAMESPACE, false) == false)
  {
    Serial.printf("Preferences initialization of namespace %s failed.\n", PREF_NAMESPACE);
    return;
  }
}

// read the serial number from EEPROM
uint16_t readSerial(void) 
{
  uint16_t serialnum;
  /*
  uint8_t serialbytes[2];
  serialbytes[0] = EEPROM.read(EE_SERIAL_ADDR);
  serialbytes[1] = EEPROM.read(EE_SERIAL_ADDR+1);
  serialnum = (serialbytes[1] << 8) | serialbytes[0];
  */

  // read the serial number from the preferences nvs and return 0 if not found
  if(prefs.isKey(PREF_SERIAL_KEY) == false)
  {
    return 0;
  }
  serialnum = prefs.getUShort(PREF_SERIAL_KEY, 0);
  return serialnum;
}

// write the serial number to EEPROM
void writeSerial(uint16_t serialnum) 
{
  uint8_t serialbytes[2];
  /*
  serialbytes[0] = serialnum & 0xFF;
  serialbytes[1] = (serialnum >> 8) & 0xFF;
  EEPROM.write(EE_SERIAL_ADDR, serialbytes[0]);
  EEPROM.write((EE_SERIAL_ADDR+1), serialbytes[1]);
  EEPROM.commit();
  */
   // write the serial number to the preferences nvs
  if(prefs.putUShort(PREF_SERIAL_KEY, serialnum) != 2)
  {
    Serial.println("Error: writeSerial: unable to write serial number to Preferences.");
  }
}

#if 0
static char ssidbuf[32] = {0}; // buffer for ssid
const char *readWifiSsid(void)
{
  const char *ssid = NULL;
  String str;
  // read the SSID from the preferences nvs and return NULL if not found
  if(prefs.isKey(PREF_WIFISSID_KEY) == false)
  {
    Serial.printf("readWifiSsid: no SSID found in Preferences.\n");
    return NULL;
  }
  str = prefs.getString(PREF_WIFISSID_KEY, "");
  //Serial.println("ssidstr = " + str);
  ssid = str.c_str();
  snprintf(ssidbuf, 32, "%s", ssid); // copy the ssid to the buffer
  //Serial.printf("readWifiSsid: SSID found in Preferences: %s\n", ssidbuf);
  return ssidbuf;
}

// write the last used wifi ssid from the EEPROM
void writeWifiSsid(const char *ssid)
{
  Serial.printf("writeWifiSsid: writing SSID %s to Preferences.\n", ssid);
  // write the SSID to the preferences nvs
  if(prefs.putString(PREF_WIFISSID_KEY, ssid) != strlen(ssid))
  {
    Serial.println("Error: writeWifiSsid: unable to write SSID to Preferences.");
  }
}

char passwdbuf[32] = {0}; // buffer for password
// read the last used wifi password from the EEPROM
const char *readWifiPass(void)
{
  const char *wifipwd = NULL;
  String str;
  // read the password from the preferences nvs and return NULL if not found
  if(prefs.isKey(PREF_WIFIPWD_KEY) == false)
  {
    Serial.printf("readWifiPass: no password found in Preferences.\n");
    return NULL;
  }
  str = prefs.getString(PREF_WIFIPWD_KEY, "");
  //Serial.println("passwdstr = " + str);
  wifipwd = str.c_str();
  snprintf(passwdbuf, 32, "%s", wifipwd); // copy the password to the buffer
  //Serial.printf("readWifiPass: password found in Preferences: %s\n", passwdbuf);
  return passwdbuf;
}

void writeWifiPass(const char *pass)
{
  Serial.printf("writeWifiPass: writing Wifi password %s to Preferences.\n", pass);
  // write the password to the preferences nvs
  if(prefs.putString(PREF_WIFIPWD_KEY, pass) != strlen(pass))
  {
    Serial.println("Error: writeWifiPass: unable to write password to Preferences.");
  }
}

// read WifiAP mode setting, return false if not found
bool readWifiModeAP(void)
{
  // read the WifiAP mode from the preferences nvs and return false if not found
  if(prefs.isKey(PREF_WIFIAP_KEY) == false)
  {
    Serial.printf("readWifiAP: no WifiAP mode key found in Preferences.\n");
    return false;
  }
  return prefs.getBool(PREF_WIFIAP_KEY, false);
}

void writeWifiModeAP(bool apmode)
{
  Serial.printf("writeWifiAP: writing WifiAP mode=%d to Preferences.\n", apmode);
  // write the WifiAP mode to the preferences nvs
  if(prefs.putBool(PREF_WIFIAP_KEY, apmode) != 1)
  {
    Serial.println("Error: writeWifiAP: unable to write WifiAP mode to Preferences.");
  }
}

// read Wifi enabled setting, return false if not found
bool readWifiEnable(void)
{
  // read the WifiAP mode from the preferences nvs and return false if not found
  if(prefs.isKey(PREF_WIFIEN_KEY) == false)
  {
    Serial.printf("readWifiEnable: no WifiEn key found in Preferences, defaulting to WiFi enabled.\n");
    return true; // default to true if not found
  }
  return prefs.getBool(PREF_WIFIEN_KEY, false);
}

void writeWifiEnable(bool enable)
{
  Serial.printf("writeWifiEnable: writing WifiEn=%d to Preferences.\n", enable);
  // write the WifiAP mode to the preferences nvs
  if(prefs.putBool(PREF_WIFIEN_KEY, enable) != 1)
  {
    Serial.println("Error: writeWifiEnable: unable to write WifiEn to Preferences.");
  }
}
#endif