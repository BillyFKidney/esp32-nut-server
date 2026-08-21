#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_log.h"

#define MANAGEMENT_DEVICE_NAME_MAX_LENGTH 63U
#define MANAGEMENT_DEVICE_HOSTNAME_MAX_LENGTH 63U

typedef struct
{
    char device_name[MANAGEMENT_DEVICE_NAME_MAX_LENGTH + 1U];
    char hostname[MANAGEMENT_DEVICE_HOSTNAME_MAX_LENGTH + 1U];
    esp_log_level_t log_level;
} ManagementDeviceConfigSnapshot;

/** Load the stored device identity and log-level settings, or defaults. */
void management_device_config_initialize(void);

/** Copy the current device identity and log-level settings into a snapshot. */
void management_device_config_snapshot(ManagementDeviceConfigSnapshot *snapshot);

/** Save a new device identity and log-level selection. */
esp_err_t management_device_config_save(const char *device_name,
                                        esp_log_level_t log_level);

/** Return the user-facing label for one ESP-IDF log level. */
const char *management_device_config_log_level_name(esp_log_level_t level);

/** Parse one device-log-level label from the management form or status UI. */
bool management_device_config_parse_log_level(const char *value,
                                             esp_log_level_t *level);
