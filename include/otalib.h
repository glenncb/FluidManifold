// OTA utiliities to write ESP32 firmware to OTA partitions of flash to allow for remote updates.
// This is a standalone library to allow writing the firmware from any source (modbus, serial, BLE, etc)
// utilizes esp-idf OTA functions to write the firmware to the OTA partitions
#pragma once

#include <stdint.h>
#include "esp_ota_ops.h"

#define OTA_REBOOT_DELAY 5
#define CONFIG_EXAMPLE_SKIP_VERSION_CHECK 1

typedef enum {
    OTA_ERR_NONE = 0,           // no error
    OTA_ERR_FSM,                // incorrect state machine sequencing
    OTA_ERR_NO_PART,            // no OTA partition is available
    OTA_ERR_IMG_TOO_LARGE,      // the image is too large for the OTA partition
    OTA_ERR_WBUF_OVF,           // the write buffer overflowed
    OTA_ERR_VER_SAME_BAD,       // the new version is the same as the last bad version which failed to boot
    OTA_ERR_VER_SAME,           // the new version is the same as the current version, no update needed
    OTA_ERR_BEGIN_FAIL,         // error in esp_ota_begin()
    OTA_ERR_WRITE_FAIL,         // error in esp_ota_write()
    OTA_ERR_IMG_SIZE_MISMATCH,  // the image size does not match the expected size
    OTA_ERR_IMG_VAL_FAIL,       // the image is not valid - failed esp_ota_end validation check
    OTA_ERR_IMG_END_FAIL,       // generic error in esp_ota_end()
    OTA_ERR_SET_BOOT_FAIL,      // error in esp_ota_set_boot_partition()
    OTA_ERR_NVS_INIT_FAIL,      // error in nvs_flash_init()
    OTA_ERR_ABORT = 0xFF        // OTA process was aborted
} ota_err_t;

// OTA state machine states
typedef enum {
    OTA_STATE_IDLE = 0,         // idle state, no OTA in progress
    OTA_STATE_SETUP,            // setup completed, no OTA in progress but ready for begin state
    OTA_STATE_BEGIN,            // start of OTA process - checks image size and that valid OTA partition exists
    OTA_STATE_WRITE_HDR,        // write header state - accumulating data to check image header, has not yet written to OTA partition
    OTA_STATE_WRITE,            // write state - use multiple writes to load data to OTA partition
    OTA_STATE_WRITE_DONE,       // full image written, waiting to call end to advance to end state
    OTA_STATE_END,              // end of ota update - indicates writes are complete, verify image and set as boot image
    OTA_STATE_UPDVLD,           // indicates the OTA image has been set to boot and is valid, ready to restart
    OTA_STATE_ABORT = 0xFF      // indicates the OTA process failed, check the error code for reason
} ota_state_t;

class ESPota {
    private: 
        esp_err_t err;
        const char *TAG = "ESPota";
        const esp_partition_t *configured_partition;
        const esp_partition_t *running_partition;
        const esp_partition_t *update_partition;
        esp_ota_handle_t update_handle;
        size_t part_size;
        size_t ota_image_size;
        size_t ota_bytes_written;
        bool ota_err, ota_write_ok; // write_ok is probably superfluous
        bool ota_header_checked;
        uint8_t ota_write_data[1024];
        int ota_wdata_len;
    public:
        ota_err_t ota_errnum;
        ota_state_t ota_state;
        ESPota(void);
        ~ESPota(void);
        // setup() - initialize OTA, determine partition, and check for OTA partition
        bool setup(void);
        // begin() - prepare to begin OTA process, check image size, flash erase the partition and enable writing
        bool begin(size_t size);
        // write() - write data to the OTA partition
        bool write(uint8_t *data, size_t len);
        // percent() - return the write completion percentage
        int percent(void);
        // bytes_written() - return the exact number of bytes actually written to flash
        // so far, for the host to verify progress after a suspected lost ack rather
        // than blindly resending (which risks a duplicate write, since this OTA
        // protocol has no per-chunk sequence/offset tracking of its own)
        size_t bytes_written(void);
        // end() - finalize the OTA process, verify the image, and set the new image as the boot image
        bool end(void);
        // mark_app_valid() - mark the new firmware as valid and disable auto rollback.
        // Also erases the OTHER app partition (the one we booted from prior to this
        // update, which - since this board's partition table has exactly two OTA
        // slots, no factory partition - is the same partition the *next* OTA update
        // will target) so a future esp_ota_begin() has nothing left to erase, moving
        // that cost off the modbus write-loop's timing entirely. Only happens once,
        // right after a genuine OTA switch-over (gated on the same PENDING_VERIFY
        // check as the rollback-cancel itself), not on every normal boot.
        void mark_app_valid(void);
        // ota_cleanup() - cleanup routine in case of an error or for a user requested abort
        void cleanup(void);
        // rollback() - force a firmware rollback to the previous version if available
        void rollback(void);
};
