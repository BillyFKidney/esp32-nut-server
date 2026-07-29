#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_chip_info.h"

#define MANAGEMENT_STATUS_NUT_VALUE_LENGTH 96U
#define MANAGEMENT_STATUS_UPS_NAME_LENGTH 32U

/** Read-only snapshot of normalized NUT state for the management response. */
typedef struct
{
    bool available;
    bool stale;
    char ups_name[MANAGEMENT_STATUS_UPS_NAME_LENGTH];
    char manufacturer[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char model[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char serial[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char status[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char battery_type[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char battery_mfr_date[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char ups_temperature[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char battery_charge[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char battery_runtime[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char battery_voltage[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char load[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char input_voltage[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char output_voltage[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char ups_power[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char ups_realpower[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
    char ups_firmware[MANAGEMENT_STATUS_NUT_VALUE_LENGTH];
} ManagementStatusNutSnapshot;

/** Read-only board and memory diagnostic snapshot for the management response. */
typedef struct
{
    esp_chip_info_t chip;
    uint32_t flash_size_bytes;
    size_t psram_size_bytes;
    size_t free_internal_heap_bytes;
    size_t free_psram_bytes;
    uint32_t minimum_free_heap_bytes;
    bool chip_temperature_available;
    float chip_temperature_celsius;
    const char *board_profile;
    const char *module_profile;
    const char *flash_mode;
    const char *flash_frequency;
    const char *psram_mode;
    int psram_frequency_mhz;
} ManagementStatusHardwareSnapshot;

/** Initialize optional hardware diagnostics. Safe to call more than once. */
void management_status_initialize_hardware_diagnostics(void);

/** Capture the current read-only NUT dstate view. */
void management_status_collect_nut_snapshot(ManagementStatusNutSnapshot *snapshot);

/** Capture the current board, memory, and optional temperature diagnostics. */
void management_status_collect_hardware_snapshot(ManagementStatusHardwareSnapshot *snapshot);

/** Return the stable display name for an ESP-IDF chip model. */
const char *management_status_chip_model_name(esp_chip_model_t model);
