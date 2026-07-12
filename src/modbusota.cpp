// modbusota.cpp - OTA firmware update over modbus for the FluidManifold driver-dispatch map.
// Ported from ESP32/DropDelayESP32/Common/modbusota.cpp and adapted to FluidManifold's
// resdriver_t architecture: instead of per-register .funcr/.funcw pointers, each OTA register is
// a DRVR_* enum whose case in modbusmap.cpp's switch() blocks calls one of the wrappers below.

#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "modbusmap.h"      // wordresource_t, multireg_wr_regnum
#include "otalib.h"
#include "modbusota.h"

// firmware update accumulation buffer (one FWDATA burst)
uint8_t fwbuf[FWBUFSZ];

// 32-bit image size / checksum, accessed 16 bits at a time over modbus
static union {
    uint32_t val;
    struct { uint16_t lo; uint16_t hi; } parts;
} fw_img_size, fw_checksum;

// the OTA class instance
static ESPota ota;

// modbusOTASetup - initialization, call once after modbusRegSetup()
void modbusOTASetup(void)
{
    // setup the OTA class (identifies partitions, checks for a valid update partition)
    ota.setup();
    // if we reached this code the currently running image booted successfully - mark it valid
    // so the bootloader does not roll back to the previous image
    ota.mark_app_valid();
    // clear the image parameters and buffer
    fw_img_size.val = 0;
    fw_checksum.val = 0;
    memset(fwbuf, 0, FWBUFSZ);
    // no pending multi-register write yet
    multireg_wr_regnum = FWBOGUSREG;
}

//----------------------------------------------------
// coils (bit resources)
//----------------------------------------------------
// FWUPDSTART read - true once ota.begin() has completed the BEGIN state
bool modr_FWUPDSTART(void) { return (ota.ota_state == OTA_STATE_BEGIN); }
// FWUPDSTART write - begin the OTA process (assumes FWSIZE was written first)
void modw_FWUPDSTART(bool val)
{
    if(!val)
        return;
    ESP_LOGI("modw_FWUPDSTART", "OTA start requested, image size %u", (unsigned) fw_img_size.val);
    // the image size must be set before begin()
    if(fw_img_size.val == 0)
    {
        ESP_LOGE("modw_FWUPDSTART", "Firmware size not set - aborting");
        ota.cleanup();
        return;
    }
    ota.begin(fw_img_size.val);
}

// FWUPDEND read - true once the image has been validated and set as the boot partition
bool modr_FWUPDEND(void) { return (ota.ota_state == OTA_STATE_UPDVLD); }
// FWUPDEND write - flush any partial final burst, then finalize/validate the OTA image
void modw_FWUPDEND(bool val)
{
    if(!val)
        return;

    // safety: the last multi-register write before end() must have targeted the FWDATA range
    if(multireg_wr_regnum != FWBOGUSREG &&
       (multireg_wr_regnum < FWDATAREG || multireg_wr_regnum >= (FWDATAREG + FWREGS)))
    {
        ESP_LOGE("modw_FWUPDEND", "last multi-register write was not to FWDATA - aborting");
        ota.cleanup();
        return;
    }

    // if the final burst was a partial block, its data is still sitting in fwbuf - flush it now
    if((multireg_wr_regnum >= FWDATAREG) && (multireg_wr_regnum < (FWDATAREG + FWREGS - 1)))
    {
        size_t sz = (multireg_wr_regnum - FWDATAREG + 1) * 2;
        ota.write(fwbuf, sz);
    }

    // finalize (verifies the image and sets it as the boot partition)
    ota.end();
}

// FWUPDABORT read - true if the OTA process has been aborted
bool modr_FWUPDABORT(void) { return (ota.ota_state == OTA_STATE_ABORT); }
// FWUPDABORT write - user-requested abort/cleanup
void modw_FWUPDABORT(bool val)
{
    if(val) {
        ota.cleanup();
        // re-initialize so the board is immediately ready for a new attempt
        // without requiring a reboot - see the comment on ESPota::setup()
        ota.setup();
    }
}

//----------------------------------------------------
// 32-bit holding registers (regvals[0]=low word, regvals[1]=high word)
//----------------------------------------------------
void modr_FWSIZE(uint16_t *regvals)     { memcpy(regvals, &fw_img_size.val, 4); }
void modw_FWSIZE(uint16_t *regvals)     { memcpy(&fw_img_size.val, regvals, 4); }
void modr_FWCHECKSUM(uint16_t *regvals) { memcpy(regvals, &fw_checksum.val, 4); }
void modw_FWCHECKSUM(uint16_t *regvals) { memcpy(&fw_checksum.val, regvals, 4); }

//----------------------------------------------------
// FWDATA multi-register block
//----------------------------------------------------
// called once per register of an FC16 burst to FWDATA.  Accumulates each 16-bit word directly
// into fwbuf (bypassing the MAXDRVREGS-sized regvals[] buffer), and when the top register of a
// full burst is written, flushes the whole block to the OTA flash partition.
// counts every full-burst flush (offset==nregs-1) - compared against the
// host's own chunk count at the WRITE_DONE transition (see otalib.cpp) to
// determine whether a phantom flush is coming from this callback firing more
// often than the host's actual FC16 write count, or from write()'s own
// internal byte accounting.
uint32_t fwdata_flush_count = 0;

uint16_t modw_FWDATAblock(uint16_t val, uint16_t regaddr, const wordresource_t *rp)
{
    int offset = regaddr - rp->num;         // rp->num == FWDATAREG
    int nregs  = (rp->len + 1) / 2;         // FWREGS

    if(offset < 0 || offset >= nregs)
    {
        ESP_LOGE("modw_FWDATAblock", "reg %d offset %d out of range (nregs %d)", regaddr, offset, nregs);
        return val;
    }

    // accumulate this word into the firmware buffer
    memcpy(fwbuf + 2*offset, &val, 2);
    // remember the highest register written so FWUPDEND can flush a partial tail
    multireg_wr_regnum = regaddr;

    // full burst complete - flush the whole block to flash
    if(offset == nregs - 1)
    {
        fwdata_flush_count++;
        ota.write(fwbuf, FWBUFSZ);
        multireg_wr_regnum = FWBOGUSREG;    // no pending partial data
    }
    return val;
}

// FWDATA readback (not used by the host during a normal update, provided for completeness)
uint16_t modr_FWDATAword(uint16_t regaddr, const wordresource_t *rp)
{
    int offset = regaddr - rp->num;
    int nregs  = (rp->len + 1) / 2;
    uint16_t val = 0;
    if(offset < 0 || offset >= nregs)
        return 0;
    memcpy(&val, fwbuf + 2*offset, 2);
    return val;
}

//----------------------------------------------------
// input registers (read-only status)
//----------------------------------------------------
uint16_t modr_FWUPDSTATE(void)   { return ota.ota_state; }
uint16_t modr_FWUPDERR(void)     { return ota.ota_errnum; }
uint16_t modr_FWUPDPERCENT(void) { return ota.percent(); }
void modr_FWUPDBYTES(uint16_t *regvals) {
    uint32_t bytes = (uint32_t) ota.bytes_written();
    memcpy(regvals, &bytes, 4);  // low word first, matches modr_FWSIZE()'s convention
}
