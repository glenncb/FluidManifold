// modbusmap.cpp - maps modbus resources to the actual resources (either direct variables or functions)
#include <stdint.h>
#include <string.h>
#include "globals.h"
#include "modbus.h"
#include "modbusmap.h"
#include "emc2302.h"
#include "mcp9808.h"
#include "dac.h"
#include "adc.h"
#include "alireg.h"
#include "esp_log.h"
#include "modbusmap_gen.h"
#include "eeprom_acc.h"
#include "serled.h"
#include "modbusota.h"

// keep track of highest register written on a multi-register write as there is no callback for write multiple registers
int multireg_wr_regnum = 0;

// return the matching resource entry associated with the coil or contact
const bitresource_t* FindBitResource(const bitresource_t *map, int num)
{
  const bitresource_t *p;
  if(!map)
    return NULL;
  for(p=map; p->num != 0; p++) // for now this is a dumb linear search as there aren't many entries
  {
    if(p->num == num)
      return p;
  } 
  // didn't match
  return NULL;
}
// return the matching resource entry associated with the input/hold register
const wordresource_t* FindWordResource(const wordresource_t *map, int num)
{
  const wordresource_t *p;
  if(!map)
    return NULL;
  for(p=map; p->num != 0; p++) // for now this is a dumb linear search as there aren't many entries
  {
    if((p->len)>2) // handle case of multiple registers with only a single map entry
    {
      //Serial.printf("FindWordResource: num %d, p->num %d, p->len %d\n", num, p->num, p->len);
      if((num >= p->num) && (num < (p->num + ((p->len+1)/2))))
        return p; // in range - return the base map entry
    }
    else if(p->num == num)
      return p;
  } 
  // didn't match
  return NULL;
}

//--------------------------------------------------------------------------------
// generic handlers for bit functions
//--------------------------------------------------------------------------------
// callback function to write a resource (coil or holding reg)
// sets the linked variable and calls the write function if defined
uint16_t mod_cbSet(TRegister* reg, uint16_t val)
{
  uint16_t regnum = reg->address.address;
  // lock the modbus semaphore
  xSemaphoreTake(modbusInUse, portMAX_DELAY);
  #if MODBUS_DEBUG
  Serial.printf("mod_cbSet: called for regnum %d, val 0x%04x\n", regnum, val);
  #endif
  ESP_LOGI("mod_cbSet", "called for regnum %d, val 0x%04x", regnum, val);
  // handle coils and contacts (bit resources seperately from word resources)
  if(reg->address.isCoil() || reg->address.isIsts())
  {
    bool coilval = COIL_BOOL(val);
    const bitresource_t *p = (reg->address.isCoil()) ? FindBitResource(coilmap, regnum) : NULL; // only coils are writeable
    if(p == NULL) // resource not found
    { 
      modPrintReg("cbSet: FindBitResource failed!:", reg, NULL, val); 
      xSemaphoreGive(modbusInUse);
      return val; 
    }
    
    if((p->drvr == DRVR_BITVAR) && (p->drvaddr != 0) && (p->acc == RES_RW)) // resource is a variable and is read/write
    {
      uint8_t *v = (uint8_t *) p->drvaddr;
      int bit = (p->drvchan)%8;
      int byte = (p->drvchan)/8;
      if(coilval)
        v[byte] |=  (1 << bit);
      else
        v[byte] &= (~(1 << bit));
    }
    else if(p->drvr != DRVR_NONE) // if a write function is defined, call it
    {
      ESP_LOGI("mod_cbSet", "calling write function for %s", p->name);
      coilval = mod_drvrSetBit(coilval, regnum, p);
    }
    
    modPrintReg("cbSet: ", reg, p->name, val);
  }
  else if(reg->address.isHreg() || reg->address.isIreg()) // check for holdreg or inputreg
  {
    const wordresource_t *p = (reg->address.isHreg()) ? FindWordResource(holdregmap, regnum) : NULL;
    if(p == NULL) // resource not found
    { 
      modPrintReg("cbSet: FindWordResource failed!:", reg, NULL, val); 
      xSemaphoreGive(modbusInUse);
      return val; 
    }
    
    if((p->drvr == DRVR_VAR) && (p->drvaddr != 0) && (p->acc == RES_RW)) // resource is a variable and is read/write
    {
      if(p->len <= 2) // single register
        memcpy((char *) p->drvaddr, &val, p->len); // copy the value to the variable
      else // multiple register - write ony the 16b word associated with this register location
      {
        int offset = regnum - p->num;
        if(offset < 0 || (offset >= (p->len+1)/2)) // out of range
        { 
          ESP_LOGE("cbSet", "BUG: offset out of range!:", regnum); 
          xSemaphoreGive(modbusInUse);
          return val; 
        } 
        // for large register writes >4 bytes, cbSet will be called once for each register in the range
        // but there is only one entry in the lookup table for the entire range
        // based on the register number, calculate the offset into the multi-word array and write the 16b value
        // associated with this sub-register
        memcpy((((char *) p->drvaddr) + 2*offset), &val, 2); // copy the value into the multi-word array
        // keep track of highest reg written since there is no callback for write multiple registers
        // we'll have to handle cleanup in the callback function
        multireg_wr_regnum = regnum + offset; // save the highest register written
      }
    }
    else if(p->drvr != DRVR_NONE) // if a write function is defined, call it
    {
      // call the driver
      val = mod_drvrSet(val, regnum, p);
    }

    modPrintReg("cbSet: ", reg, p->name, val);
  }   
  else
  { modPrintReg("cbSet: bug - unknown regtype!:", reg, NULL, val); }

  // return the value written
  xSemaphoreGive(modbusInUse);
  return val;
}
// callback function to read a resource 
// performs the read callback first, then stores this value into the linked variable
uint16_t mod_cbGet(TRegister* reg, uint16_t val) // note that val is not used
{
  int regnum = reg->address.address;

  // lock the modbus semaphore
  xSemaphoreTake(modbusInUse, portMAX_DELAY);

  #if MODBUS_DEBUG
  Serial.printf("mod_cbGet: called for regnum %d\n", regnum);
  #endif
  ESP_LOGI("mod_cbGet", "called for regnum %d", regnum);

  if(reg->address.isCoil() || reg->address.isIsts()) // bit type resource (coil/contact)
  {
    int bitval = -1;
    const bitresource_t *p = (reg->address.isCoil()) ? FindBitResource(coilmap, regnum) : FindBitResource(contmap, regnum); // coil vs. contact
    if(p == NULL) // resource not found
    { 
      modPrintReg("cbGet: FindBitResource failed!:", reg, NULL, val); 
      xSemaphoreGive(modbusInUse);
      return val; 
    }

    // if a read function is defined, call it to get the value
    if((p->drvr != DRVR_NONE) && p->drvr != DRVR_BITVAR) // read functiona has highest priority 
    {
        bitval = mod_drvrGetBit(regnum, p); 
    }
    else if((p->drvr == DRVR_BITVAR) && (p->drvaddr != 0)) // resource has a variable (2nd priority)
    {
      uint8_t *v = (uint8_t *) p->drvaddr; 
      int byte = (p->drvchan)/8; // find the byte location of the bit
      int bit = (p->drvchan)%8;  // bit location within that byte
      
      if(bitval >= 0) // bitval is already known, so update the varp to match the bitval
      {
        if(bitval)
          v[byte] |=  (1 << bit);
        else
          v[byte] &= (~(1 << bit));
      }
      else // bitval is not known, so read it from the varp
      {
        bitval = (v[byte] & (1 << bit)) ? 1 : 0;
      }
    }
    // convert to a modbus valid coil/contact read value
    if(bitval>=0)
      val = COIL_VAL(bitval);
    modPrintReg("cbGet: ", reg, p->name, val);
  }
  else if(reg->address.isHreg() || reg->address.isIreg()) // check for holdreg or inputreg
  {
  // handle word resources (holding / input registers)
    int32_t wdval = -1;
    const wordresource_t *p = (reg->address.isHreg()) ? FindWordResource(holdregmap, regnum) : FindWordResource(inregmap, regnum);
    if(p == NULL) // resource not found
    { 
      modPrintReg("cbGet: FindWordResource failed!:", reg, NULL, val); 
      xSemaphoreGive(modbusInUse);
      return val; 
    }

    // if a read function is defined, call it to get the value
    if((p->drvr != DRVR_NONE) && p->drvr != DRVR_VAR) // read functiona has highest priority 
    {
      wdval = mod_drvrGet(regnum, p, reg->address.isHreg());
      val = wdval;
    }
    else if((p->drvr == DRVR_VAR) && (p->drvaddr != 0)) // resource has a variable (2nd priority)
    {
      if(wdval>0) // value was updated by function, copy this to the variable pointer
      {
        uint8_t *v = (uint8_t *) p->drvaddr; 
        if(p->len <= 2) // single register
          memcpy(v, &wdval, p->len); // copy the value to the variable
        else // multiple register - update ony the 16b word associated with this register location
        {
          int offset = regnum - p->num;
          if(offset < 0 || (offset >= (p->len+1)/2)) // out of range
          { 
            ESP_LOGE("cbGet", "BUG: offset out of range!:", regnum); 
            xSemaphoreGive(modbusInUse);
            return val; 
          } 
          memcpy((v + 2*offset), &wdval, 2); // copy the function return value into the multi-word array
        }
        val = wdval;
      }
      else // value was not updated by function, so read it from the variable pointer
      {
        uint8_t *v = (uint8_t *) p->drvaddr; 
        if(p->len <= 2) // single register
          memcpy(&val, v, p->len); // copy the value to the variable
        else // multiple register - read only the 16b word associated with this register location
        {
          int offset = regnum - p->num;
          if(offset < 0 || (offset >= (p->len+1)/2)) // out of range
          { 
            ESP_LOGE("cbGet", "BUG: offset out of range!:", regnum); 
            xSemaphoreGive(modbusInUse);
            return val; 
          } 
          memcpy(&val, (v + 2*offset), 2); // copy the value from the multi-word array
        }
      }
    }
    modPrintReg("cbGet: ", reg, p->name, val);
  }
  else
  { modPrintReg("cbGet: bug - unknown regtype!:", reg, NULL, val); }

  xSemaphoreGive(modbusInUse);
  return val; // default if there was no mapping
}

// process the user specified modbus maps and create the corresponding ModbusRTU registers
// process the user specified modbus maps and create the corresponding ModbusRTU registers
#if USE_MODBUS_RTU
void modbusRegSetup(ModbusRTU *mb)
#else
void modbusRegSetup(ModbusIP *mb)
#endif
{
  int ncoil=0;
  int ncont=0;
  int nhold=0;
  int nin=0;

  // add user specified coils
  for(ncoil=0; coilmap[ncoil].num != 0; ncoil++)
  {
    int num = coilmap[ncoil].num;
    mb->addCoil(num, coilmap[ncoil].init); // add the coil to the modbus map and init
    mb->onGetCoil(num, mod_cbGet);  // define the read callback
    mb->onSetCoil(num, mod_cbSet);  // define the write callback
    // FIXME - need to make sure the intial value and callback mapped resources are in sync
  }
  // add user specified discrete inputs (contacts)
  for(ncont=0; contmap[ncont].num != 0; ncont++)
  {
    int num = contmap[ncont].num;
    mb->addIsts(num); // add the coil to the modbus map
    mb->onGetIsts(num, mod_cbGet);  // define the read callback
    // no write callback for contacts
  }
  // add user specified holding registers
  for(nhold=0; holdregmap[nhold].num != 0; nhold++)
  {
    int num = holdregmap[nhold].num;
    int len = holdregmap[nhold].len;
    if(len > 2)
      mb->addHreg(num, holdregmap[nhold].init, ((len+1)/2)); // add the coil to the modbus map and initialize
    else 
      mb->addHreg(num, holdregmap[nhold].init); // add the coil to the modbus map and initialize
    // special case to handle read_multiple_registers with length >2 used by firmware update
    if(len > 2)
      mb->onGetHreg(num, mod_cbGet, ((len+1)/2));  // define the read callback
    else
      mb->onGetHreg(num, mod_cbGet);  // define the read callback
    // special case to handle write_multiple_registers with length >2 used by firmware update
    if(len > 2)
      mb->onSetHreg(num, mod_cbSet, ((len+1)/2));  // define the write callback
    else
      mb->onSetHreg(num, mod_cbSet);  // define the write callback
    // FIXME - need to make sure the intial value and callback mapped resources are in sync
  }
  // add user specified input registers
  for(nin=0; inregmap[nin].num != 0; nin++)
  {
    int num = inregmap[nin].num;
    int len = inregmap[nin].len;
    if(len > 2)
      mb->addIreg(num, inregmap[nin].init, ((len+1)/2)); // add the coil to the modbus map and initialize
    else 
      mb->addIreg(num, inregmap[nin].init); // add the coil to the modbus map and initialize
    if(len > 2)
      mb->onGetIreg(num, mod_cbGet, ((len+1)/2));  // define the read callback
    else
      mb->onGetIreg(num, mod_cbGet);  // define the read callback
    // no write callback for input registers
  }

  Serial.printf("modbusRegSetup: added %d coils, %d contacts, %d holding regs, %d input regs\n", ncoil, ncont, nhold, nin);

  // setup the modbus referenced GPIOs
  modGpioInit();
  Serial.printf("modbusRegSetup: initialized GPIOtab resources\n");
}

// print out a modbus register (for debug)
void modPrintReg(const char* prefix, TRegister* reg, const char *name=NULL, uint16_t val=0)
{
#if MODBUS_DEBUG
  const char* regtypename[] = {"Coil", "Cont", "InReg", "HoldReg"};
  Serial.printf("%s: %s[%d] \"%s\" cur=0x%04x new=0x%04x\n", 
    prefix, regtypename[reg->address.type], reg->address.address, name ? name : "NULL", reg->value, val);
#endif
}

// driver handler for bit resources
bool mod_drvrGetBit(uint16_t regaddr, const bitresource_t *rp)
{
  resdriver_t drvr = rp->drvr;
  uint32_t drvaddr = rp->drvaddr;
  uint8_t drvchan = rp->drvchan;
  bool bitval = false;
  switch(drvr)
  {
    case DRVR_GPIO:
      bitval = pinGPIO[drvchan].read();
      break;
    case DRVR_FWUPDSTART: bitval = modr_FWUPDSTART(); break;
    case DRVR_FWUPDEND:   bitval = modr_FWUPDEND();   break;
    case DRVR_FWUPDABORT: bitval = modr_FWUPDABORT(); break;
    default:
      Serial.printf("mod_drvrGetBit: Error: driver %d not implemnted\n", drvr);
      return false;
  }
  return bitval;
}

bool mod_drvrSetBit(bool bitval, uint16_t regaddr, const bitresource_t *rp)
{
  resdriver_t drvr = rp->drvr;
  uint32_t drvaddr = rp->drvaddr;
  uint8_t drvchan = rp->drvchan;
  switch(drvr)
  {
    case DRVR_GPIO:
      pinGPIO[drvchan].setLevel(bitval);
      break;
    case DRVR_FWUPDSTART: modw_FWUPDSTART(bitval); break;
    case DRVR_FWUPDEND:   modw_FWUPDEND(bitval);   break;
    case DRVR_FWUPDABORT: modw_FWUPDABORT(bitval); break;
    default:
      Serial.printf("mod_drvrSetBit: Error: driver %d not implemnted\n", drvr);
  }
  return bitval;
}

// driver handler for word resources

// use write_pending to track that a multi-register driver write is in progress
// while a write is pending, return data from the regvals array instead of the driver
// back to back reads of the base read register will clear the write_pending flag and
// allow the driver to be called again
bool drvr_write_pending = false;
uint16_t regvals[MAXDRVREGS];
uint16_t mod_drvrGet(uint16_t regaddr, const wordresource_t *rp, bool ishreg)
{
  uint16_t val = 0;
  resdriver_t drvr = rp->drvr;
  uint32_t drvaddr = rp->drvaddr;
  int8_t drvchan = rp->drvchan;

  int offset = regaddr - rp->num;

  // FWDATA firmware stream bypasses the MAXDRVREGS-sized regvals[] buffer (readback, non-critical)
  if(drvr == DRVR_FWDATA)
    return modr_FWDATAword(regaddr, rp);

#if MODBUS_DEBUG
  Serial.printf("mod_drvrGet: regaddr %d, offset %d, len %d, ishreg %d\n", regaddr, offset, rp->len, ishreg);
#endif
  // check if the register of a multi-register write is in range
  if((offset < 0) || (offset >= (rp->len+1)/2) || (offset >= MAXDRVREGS)) // out of range
  { 
    if(offset >= MAXDRVREGS) { ESP_LOGE("mod_drvrGet", "BUG: register number %d of register group %s: offset %d exceeds MAXDRVREGS (%d)", regaddr, rp->name, offset, MAXDRVREGS); }
    else { ESP_LOGE("mod_drvrSet", "BUG: register number %d of register group %s: offset %d out of range!:", regaddr, rp->name, offset); }
    return val; 
  }

  // check if this read is for the first register in a multi-register read - trigger the action only on the first register read
  if(regaddr == rp->num && !drvr_write_pending)
  {

#if MODBUS_DEBUG
    Serial.printf("mod_drvrGet: invoking driver %d for base register %d\n", drvr, regaddr);
#endif

    // regvals now contains the complete data needed by the function call
    switch(drvr)
    {
      case DRVR_FLASH:
        val = readSerial();
        serialno = val; // update the serial number
        break;
      case DRVR_FAN:
        val = modr_Fan(&val, drvaddr, drvchan);
        break;
      case DRVR_TEMP:
        modr_Temp(regvals, drvaddr, drvchan);
        break;
      case DRVR_DAC:
        modr_I2CDAC(regvals, drvaddr, drvchan);
        break;
      case DRVR_ADC:
        modr_ADC(regvals, drvaddr, drvchan);
        if(drvchan<0) // raw read for ADC, return the raw adc code 16b value instead of the float value
          val = regvals[0];
        break;
      case DRVR_ALIREG:
        modr_ALIREG(regvals, drvaddr, drvchan);
        break;
      case DRVR_SERLED:
        modr_SERLED(regvals, drvaddr, drvchan);
        val = regvals[0];
        break;
      case DRVR_FWSIZE:       modr_FWSIZE(regvals);      break;
      case DRVR_FWCHECKSUM:   modr_FWCHECKSUM(regvals);  break;
      case DRVR_FWUPDSTATE:   val = modr_FWUPDSTATE();   break;
      case DRVR_FWUPDERR:     val = modr_FWUPDERR();     break;
      case DRVR_FWUPDPERCENT: val = modr_FWUPDPERCENT(); break;
      case DRVR_FWUPDBYTES:   modr_FWUPDBYTES(regvals);  break;
      default:
        Serial.printf("cbSet: Error: driver %d not implemented for register %s\n", drvr, rp->name);
        break;
    }
  }

  // if this is a multi-register read, regvals should contain the data from the driver, return that value
  // this assumes that registers are always read starting with the first register in the group
  if(rp->len > 2)
  {
    val = regvals[offset];
    drvr_write_pending = false; // clear the write_pending flag so that subsequent calls will clear the flag
  }
  // return the value 
  return val;
}

uint16_t mod_drvrSet(uint16_t val, uint16_t regaddr, const wordresource_t *rp)
{
  resdriver_t drvr = rp->drvr;
  uint32_t drvaddr = rp->drvaddr;
  int8_t drvchan = rp->drvchan;
  int offset = regaddr - rp->num;

  // FWDATA firmware stream bypasses the MAXDRVREGS-sized regvals[] buffer (200-byte block)
  if(drvr == DRVR_FWDATA)
    return modw_FWDATAblock(val, regaddr, rp);

  // check if the register of a multi-register write is in range
  if((offset < 0) || (offset >= (rp->len+1)/2) || (offset >= MAXDRVREGS)) // out of range
  {
    if(offset >= MAXDRVREGS) { ESP_LOGE("mod_drvrSet", "BUG: register number %d of register group %s: offset %d exceeds MAXDRVREGS (%d)", regaddr, rp->name, offset, MAXDRVREGS); }
    else { ESP_LOGE("mod_drvrSet", "BUG: register number %d of register group %s: offset %d out of range!:", regaddr, rp->name, offset); }
    return val; 
  }

  if(rp->len > 2)
  {
    drvr_write_pending = true; // set the write_pending flag to indicate that a multi-register write is in progress
  }

  // check if this write is to the last register in a multi-register write - trigger the action only on the last register write
  if(regaddr == (rp->num + ((rp->len+1)/2) - 1))
  {

#if MODBUS_DEBUG
    Serial.printf("mod_drvrSet: invoking driver %d for base register %d\n", drvr, rp->num);
#endif

    regvals[offset] = val; // save the value for the final register write
    drvr_write_pending = false; // clear the write_pending flag to indicate that a multi-register write is done

    // regvals now contains the complete data needed by the function call
    switch(drvr)
    {
      case DRVR_FLASH:
        if(boardctl.bits.debug)
        { 
          Serial.printf("cbSet: Writing serial number to flash: %d\n", val);
          writeSerial(val); 
        }
        else Serial.printf("cbSet: Error: Flash write disabled - must be in debug mode to write serial number.\n");
        // read the serial number back into val, if it mismatches, modbus can indicate an error that the serial number write failed
        val = readSerial();
        serialno = val; // update the serial number
        break;
      case DRVR_FAN:
        modw_Fan(&val, drvaddr, drvchan);
        break;
      case DRVR_DAC:
        modw_I2CDAC(regvals, drvaddr, drvchan);
        break;
      case DRVR_ALIREG:
        modw_ALIREG(regvals, drvaddr, drvchan);
        break;
      case DRVR_SERLED:
        modw_SERLED(regvals, drvaddr, drvchan);
        break;
      case DRVR_FWSIZE:     modw_FWSIZE(regvals);     break;
      case DRVR_FWCHECKSUM: modw_FWCHECKSUM(regvals); break;
      default:
        Serial.printf("cbSet: Error: driver %d not implemented for register %s\n", drvr, rp->name);
        break;
    }
  }
  else
  {
    // save the value in regvals array until the final register write
    regvals[offset] = val;
  }

  // return the value we want to store in the destination register
  return val;
}
