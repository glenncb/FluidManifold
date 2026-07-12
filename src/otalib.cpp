// OTA utilities to write ESP32 firmware to OTA partitions of flash to allow for remote updates.
// This is a standalone library to allow writing the firmware from any source (modbus, serial, BLE, etc)
// utilizes esp-idf OTA functions to write the firmware to the OTA partitions

#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "errno.h"
#include "otalib.h"

// defined in modbusota.cpp - counts full-burst FWDATA flushes, for comparing
// against the host's own chunk count when debugging the WRITE_DONE-too-early
// issue (see the print in write() below)
extern uint32_t fwdata_flush_count;

// OTA class constructor

ESPota::ESPota(void) {
    // Constructor
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    update_handle = 0 ;
    update_partition = NULL;
    part_size = 0;
    ota_write_ok = false;
    ota_err = false;
    ota_errnum = OTA_ERR_NONE;
    ota_state = OTA_STATE_IDLE;
    err = ESP_OK;

#if 0
    // set the log level for the OTA class
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    //Serial.setDebugOutput(true);
#endif
};

ESPota::~ESPota(void) {
    // Destructor
    // free the update_partition handle
};

// setup() - initializes the OTA class
// identify the proper partition for writing the firmware
// determine the maximum size of the firmware that can be written
// perform checks to make sure we are able to perform OTA updates
// return false if the OTA partition is not available
// can also be called to re-initialize after FSM is in aborted state
bool ESPota::setup() {
    // put things in safe state
    update_handle = 0 ;
    update_partition = NULL;
    part_size = 0;
    ota_write_ok = false;
    ota_err = false;
    ota_errnum = OTA_ERR_NONE;
    ota_state = OTA_STATE_IDLE;
    err = ESP_OK;

    // check the current boot partition information
    configured_partition = esp_ota_get_boot_partition();
    running_partition = esp_ota_get_running_partition();

    if (configured_partition != running_partition) {
        ESP_LOGW(TAG, "WARN: Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x. ",
                 configured_partition->address, running_partition->address);
        ESP_LOGW(TAG, "This can happen if either the OTA boot data or preferred boot image become corrupted somehow.");
    }

    ESP_LOGI(TAG, "Running partition is \"%s\": type %d subtype %d (offset 0x%08x)",
             running_partition->label, running_partition->type, running_partition->subtype, running_partition->address);

    // get the update partition information
    update_partition = esp_ota_get_next_update_partition(NULL);
    if(update_partition != NULL)
        ESP_LOGI(TAG, "Selected target OTA update partition \"%s\": type %d subtype %d (offset 0x%08x)",
             update_partition->label, update_partition->type, update_partition->subtype, update_partition->address);
    else
    {
        ESP_LOGE(TAG, "No update partition found.");
        ota_err = true; // set error flag so that subsequent calls to OTA functions will fail
        ota_errnum = OTA_ERR_NO_PART;
        ota_state = OTA_STATE_ABORT;
        return false;
    }

    // Setup complete and all checks passed, return true
    ota_state = OTA_STATE_SETUP;
    return true;
};

// begin() called at start of ota update process
// flash erases the targeted OTA partition
// determine the total size of the firmware to be written
// size - size of the firmware to be written
bool ESPota::begin(size_t size) {

    // ESP_LOGE (not LOGI) so this survives CORE_DEBUG_LEVEL=1 builds - OTA
    // attempts are rare/low-volume, worth always seeing this checkpoint
    ESP_LOGE(TAG, "begin() called - image size %d", size);

    ota_image_size = size;

    if(ota_state != OTA_STATE_SETUP) {
        ESP_LOGE(TAG, "begin() called out of sequence");
        ota_err = true; // set error flag so that subsequent calls to OTA functions will fail
        ota_errnum = OTA_ERR_FSM;
        ota_state = OTA_STATE_ABORT;
        ota_write_ok = false;
        return false;
    }

    // check the size of the firmware to be written is less than the partition size
    part_size = update_partition->size;
    if(size > part_size)
    {
        ESP_LOGE(TAG, "Firmware image size %d is larger than partition size %d", size, part_size);
        ota_err = true; // set error flag so that subsequent calls to OTA functions will fail
        ota_state = OTA_STATE_ABORT;
        ota_errnum = OTA_ERR_IMG_TOO_LARGE;
        ota_write_ok = false;
        return false;
    }

    // zero the write data buffer
    ota_wdata_len = 0;
    memset(ota_write_data, 0, sizeof(ota_write_data));

    // reset the running byte counter so a second OTA attempt in the same boot session (e.g. after
    // an abort + retry) starts fresh - end-of-image detection and the final size check depend on it
    ota_bytes_written = 0;

    // Hold off on starting the OTA process until we've received the full header and can check image integrity
    ota_header_checked = false;

    // esp_ota_begin will erase the partition and prepare for writing
    //esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);

    ota_state = OTA_STATE_BEGIN;
    return true;
};

// call write repeatedly as data is available
bool ESPota::write(uint8_t *data, size_t len) {

    // Write data to the OTA partition
    ESP_LOGI(TAG, "write() called with len %d", len);

    // check if the OTA process has encountered an error before this point
    if (ota_err) {
        ESP_LOGE(TAG, "OTA in error state - write ignored");
        return false;
    }

    // check that we are in a valid state to perform writes
    if(ota_state != OTA_STATE_BEGIN && ota_state != OTA_STATE_WRITE_HDR && ota_state != OTA_STATE_WRITE) {
        ESP_LOGE(TAG, "write() called out of sequence from state %d", ota_state);
        ota_err = true; // set error flag so that subsequent calls to OTA functions will fail
        ota_errnum = OTA_ERR_FSM;
        ota_state = OTA_STATE_ABORT;
        ota_write_ok = false;
        return false;
    }

    // initially buffer up enough data to check that the image header is valid, then begin OTA and write the data
    // after the check has been performed, directly write the data to the OTA partition
    if (len > 0) {
        if (ota_header_checked == false) {
            ota_state = OTA_STATE_WRITE_HDR;
            // header not yet written, just append the data to the buffer
            if (ota_wdata_len + len <= sizeof(ota_write_data)) {
                memcpy(&ota_write_data[ota_wdata_len], data, len);
                ota_wdata_len += len;
            } else {
                // buffer overflowed - this should not happen
                ESP_LOGE(TAG, "BUG: write data buffer overflow occurred before header could be checked");
                ota_err = true; // set error flag so that subsequent calls to OTA functions will fail
                ota_errnum = OTA_ERR_WBUF_OVF;
                ota_state = OTA_STATE_ABORT;
                ota_write_ok = false;
                return false;
            }

            /// check the header if we've received sufficient data
            if (ota_wdata_len > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                esp_app_desc_t new_app_info;

                // since we've received enough data to check the header, mark the header as checked
                ota_header_checked = true;
                // advance the state to OTA_STATE_WRITE
                ota_state = OTA_STATE_WRITE;

                // check current version with downloading
                memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                // ESP_LOGE (not LOGI/LOGW) so these survive CORE_DEBUG_LEVEL=1 builds
                ESP_LOGE(TAG, "New firmware version: %s", new_app_info.version);

                esp_app_desc_t running_app_info;
                if (esp_ota_get_partition_description(running_partition, &running_app_info) == ESP_OK) {
                    ESP_LOGE(TAG, "Running firmware version: %s", running_app_info.version);
                }

                const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                esp_app_desc_t invalid_app_info;
                if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                    ESP_LOGE(TAG, "Last invalid partition firmware version: %s", invalid_app_info.version);
                }

                // check current version with last invalid partition
                /*
                if (last_invalid_app != NULL) {
                    if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGE(TAG, "New version is the same as invalid version %s which has previously failed boot.", invalid_app_info.version);
                        ESP_LOGE(TAG, "OTA aborted - firmware will remain at previous version.");
                        ota_errnum = OTA_ERR_VER_SAME_BAD;
                        ota_state = OTA_STATE_ABORT;
                        ota_err = true;
                        cleanup();
                        return false;
                    }
                }
                */
#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
                if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                    ESP_LOGE(TAG, "Current running version is the same as a new. Update canceled.");
                    cleanup();
                    return false;
                }
#endif
                // Tried bulk-erasing here (passing ota_image_size instead of
                // OTA_WITH_SEQUENTIAL_WRITES) per ESP-IDF's OTA performance
                // tuning guide, to remove per-sector erase latency from the
                // write-loop's timing. Reverted: measured on real hardware,
                // this made the call that crosses the header-detection
                // threshold (always the 2nd FWDATA chunk) block for >2s
                // (confirmed via timestamps: esp_ota_begin succeeded 2.17s
                // after this point was reached), reliably exceeding
                // novafwupd's MB_TIMEOUT and failing every transfer. Back to
                // incremental per-sector erasing, which spreads that cost
                // across many small writes instead of concentrating it into
                // one call the host can't wait out.
                err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                    ota_errnum = OTA_ERR_BEGIN_FAIL;
                    ota_state = OTA_STATE_ABORT;
                    ota_err = true;
                    cleanup();
                    return false;
                }
                ESP_LOGE(TAG, "esp_ota_begin succeeded");

                // set the FSM sate to OTA_STATE_WRITE to indicate we've started writes to the OTA partition
                ota_state = OTA_STATE_WRITE;

                // write the header data to the OTA partition
                err = esp_ota_write( update_handle, (const void *)ota_write_data, ota_wdata_len);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                    ota_errnum = OTA_ERR_WRITE_FAIL;
                    ota_state = OTA_STATE_ABORT;
                    ota_err = true;
                    cleanup();
                    return false;
                }
                // update the write counter and clear the temp buffer
                ota_bytes_written += ota_wdata_len;
                // fires once (the header-buffer flush), safe checkpoint print
                ESP_LOGI(TAG, "write(): header flush - wrote %d bytes, ota_bytes_written now %u, fwdata_flush_count=%u",
                    ota_wdata_len, (unsigned) ota_bytes_written, (unsigned) fwdata_flush_count);
                ota_wdata_len = 0;
                memset(ota_write_data, 0, sizeof(ota_write_data));
                // BUG FIX: this call's data was already written above as part of
                // the combined header-buffer flush - without this return, execution
                // fell through into the direct-write section below and wrote this
                // same call's `data`/`len` to flash AGAIN, double-counting it in
                // ota_bytes_written (the flat +200-byte overcount that caused
                // WRITE_DONE to trigger one chunk early on every transfer).
                return true;
            } else {
                // we haven't yet received enough data to check the header, so just return
                return true;
            }
        }

        // at this point, header should be valid, so write the data to the OTA partition 

        // double check we are in the correct FSM state
        if(ota_state != OTA_STATE_WRITE) {
            ESP_LOGE(TAG, "write() called out of sequence from state %d", ota_state);
            ota_err = true; // set error flag so that subsequent calls to OTA functions will fail
            ota_errnum = OTA_ERR_FSM;
            ota_state = OTA_STATE_ABORT;
            ota_write_ok = false;
            return false;
        }

        // perform the write to the OTA partition
        err = esp_ota_write( update_handle, (const void *)data, len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "write(): write failed (%s)", esp_err_to_name(err));
            ota_errnum = OTA_ERR_WRITE_FAIL;
            ota_state = OTA_STATE_ABORT;
            ota_err = true;
            cleanup();
            return false;
        }
        ota_bytes_written += len;
        // print a message indicate write progress
        ESP_LOGI(TAG, "wrote %d bytes", len);
    }

    // write successful

    if(ota_bytes_written >= ota_image_size) {
        // ESP_LOGE (not LOGI) so this survives CORE_DEBUG_LEVEL=1 builds - fires
        // exactly once (state transition), safe to always show. Debugging a
        // suspected off-by-one where ota_bytes_written reaches ota_image_size
        // before all of the host's chunks (esp. a non-200-byte-multiple final
        // chunk) have actually been sent.
        ESP_LOGE(TAG, "write(): reached WRITE_DONE - ota_bytes_written=%u ota_image_size=%u (this call's len=%u) fwdata_flush_count=%u",
            (unsigned) ota_bytes_written, (unsigned) ota_image_size, (unsigned) len, (unsigned) fwdata_flush_count);
        // we've written the full image, set the state to OTA_STATE_WRITE_DONE
        ota_state = OTA_STATE_WRITE_DONE;
    }

    return true;
};

int ESPota::percent() {
    // return the write completion percentage
    return ota_image_size ? ota_bytes_written*100/ota_image_size : 0;
};

size_t ESPota::bytes_written() {
    return ota_bytes_written;
};

// write is complete, finalize the write process
// verify the firmware was written correctly
// set the new firmware as the boot firmware if all checks are successful
// return false if the firmware was not written correctly
bool ESPota::end() {

    if(ota_state != OTA_STATE_WRITE_DONE) {
        ESP_LOGE(TAG, "end() called out of sequence");
        ota_err = true; // set error flag so that subsequent calls to OTA functions will fail
        ota_errnum = OTA_ERR_FSM;
        ota_state = OTA_STATE_ABORT;
        ota_write_ok = false;
        return false;
    }
    
    // End
    ESP_LOGI(TAG, "end() called");
    ota_state = OTA_STATE_END;
    
    // check if any errors occurred during the OTA process
    if(ota_err) {
        ESP_LOGE(TAG, "end(): OTA errors occurred.  OTA process aborted.");
        ota_err = true;
        ota_errnum = OTA_ERR_ABORT;
        ota_state = OTA_STATE_ABORT;
        cleanup();
        return false;
    }

    // check that the size of the firmware written matches the expected size
    if(ota_bytes_written != ota_image_size) {
        ESP_LOGE(TAG, "end(): OTA image size mismatch.  Expected %d, wrote %d", ota_image_size, ota_bytes_written);
        ota_errnum = OTA_ERR_IMG_SIZE_MISMATCH;
        ota_err = true;
        ota_state = OTA_STATE_ABORT;
        cleanup();
        return false;
    } else {
        ESP_LOGI(TAG, "end(): Successfully wrote OTA image of size %d bytes", ota_bytes_written);
    }

    // finalize the OTA process
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "esp_ota_end: Image validation failed, image is corrupted");
            ota_errnum = OTA_ERR_IMG_VAL_FAIL;
            ota_state = OTA_STATE_ABORT;
            ota_err = true;
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            ota_errnum = OTA_ERR_IMG_END_FAIL;
            ota_state = OTA_STATE_ABORT;
            ota_err = true;
        }
        cleanup();
        return false;
    }

    // set the new firmware as the boot firmware
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        ota_errnum = OTA_ERR_SET_BOOT_FAIL;
        ota_state = OTA_STATE_ABORT;
        ota_err = true;
        cleanup();
    }

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "end(): NVS flash initialization failed (%s)!", esp_err_to_name(err));
        ota_errnum = OTA_ERR_NVS_INIT_FAIL;
        ota_state = OTA_STATE_ABORT;
        ota_err = true;
        return false;
    }

    // OTA successful
    ota_state = OTA_STATE_UPDVLD;
    return true;
};

// cleanup routine in case of an error
void ESPota::cleanup() {
    // Cleanup
    esp_ota_abort(update_handle);
    ota_err = true;
    // if this abort was requested by the user, set the error number to OTA_ABORT
    if(ota_errnum == OTA_ERR_NONE)
        ota_errnum = OTA_ERR_ABORT;

    ota_state = OTA_STATE_ABORT;
    ota_write_ok = false;
    ESP_LOGE(TAG, "cleanup(): OTA errors occurred.  OTA process aborted.");
};

// Marks the boot partition as valid after an OTA
// call this function after a successful boot of the OTA image to mark the new firmware as valid
// and disable auto rollback to the previous firmware
void ESPota::mark_app_valid() {
    
    // get the current running partition
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        // check if this is in a pending verify state
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // assume that if we booted and reached this code, things are healthy, mark as good
            ESP_LOGI(TAG, "Marking firmware image as good and canceling rollback");
            esp_ota_mark_app_valid_cancel_rollback();

            // now that we're committed to this image, pre-erase the standby
            // partition (the one we just booted away from) so the next OTA
            // update's esp_ota_begin() has nothing to erase - this runs before
            // modbusStartServer() brings up the request-processing task, so
            // there's no live modbus transaction for it to block
            ESP_LOGI(TAG, "Pre-erasing standby OTA partition for the next update");
            esp_err_t erase_err = esp_ota_erase_last_boot_app_partition();
            if (erase_err != ESP_OK) {
                ESP_LOGW(TAG, "esp_ota_erase_last_boot_app_partition() failed (%s) - "
                              "next OTA update will just erase inline as before",
                    esp_err_to_name(erase_err));
            }
        }
    }
};

void ESPota::rollback() {
    ESP_LOGI(TAG, "rollback(): Forcing rollback to the previous firmware version and rebooting");
    ESP_LOGE(TAG, "rollback(): REBOOTING IN %d SECONDS...", OTA_REBOOT_DELAY);
    // wait to allow user to see message before we reboot
    vTaskDelay(OTA_REBOOT_DELAY * 1000 / portTICK_PERIOD_MS);
    esp_ota_mark_app_invalid_rollback_and_reboot();
};