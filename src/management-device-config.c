/** @file management-device-config.c @brief Persist and normalize the device display name and log level. @see management-device-config.h, nvs.h, esp_log.h */
#include "management-device-config.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "nvs.h"

#define TAG "nut-management"

#define MANAGEMENT_DEVICE_CONFIG_NAMESPACE "management"
#define MANAGEMENT_DEVICE_CONFIG_KEY "device-config"
#define MANAGEMENT_DEVICE_CONFIG_VERSION 1U
#define MANAGEMENT_DEVICE_DEFAULT_NAME "ESP32-NUT"
#define MANAGEMENT_DEVICE_DEFAULT_HOSTNAME "esp32-nut"

typedef struct
{
    uint32_t version;
    int32_t log_level;
    char device_name[MANAGEMENT_DEVICE_NAME_MAX_LENGTH + 1U];
} ManagementDeviceConfigRecord;

static portMUX_TYPE management_device_config_lock = portMUX_INITIALIZER_UNLOCKED;
static ManagementDeviceConfigRecord management_device_config_record = {
    .version = MANAGEMENT_DEVICE_CONFIG_VERSION,
    .log_level = ESP_LOG_INFO,
    .device_name = MANAGEMENT_DEVICE_DEFAULT_NAME,
};
static bool management_device_config_initialized;

_Static_assert(sizeof(MANAGEMENT_DEVICE_CONFIG_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "Device-config namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_DEVICE_CONFIG_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "Device-config key exceeds the ESP-IDF limit");

static bool management_device_name_copy_trimmed(const char *source, char *destination,
                                                size_t destination_size)
{
    if (source == NULL || destination == NULL || destination_size == 0)
    {
        return false;
    }

    const unsigned char *start = (const unsigned char *)source;
    while (*start != '\0' && isspace(*start))
    {
        start++;
    }

    const unsigned char *end = start + strlen((const char *)start);
    while (end > start && isspace(end[-1]))
    {
        end--;
    }

    const size_t length = (size_t)(end - start);
    if (length == 0 || length > MANAGEMENT_DEVICE_NAME_MAX_LENGTH)
    {
        return false;
    }

    for (size_t index = 0; index < length; index++)
    {
        if (iscntrl(start[index]))
        {
            return false;
        }
    }

    if (length >= destination_size)
    {
        return false;
    }

    memcpy(destination, start, length);
    destination[length] = '\0';
    return true;
}

static void management_device_build_hostname(const char *device_name, char *hostname,
                                            size_t hostname_size)
{
    if (hostname == NULL || hostname_size == 0)
    {
        return;
    }

    size_t used = 0;
    bool previous_separator = true;
    for (const unsigned char *cursor = (const unsigned char *)device_name;
         cursor != NULL && *cursor != '\0' && used + 1U < hostname_size; cursor++)
    {
        const unsigned char character = *cursor;
        if (character < 0x80U && isalnum(character))
        {
            hostname[used++] = (char)tolower(character);
            previous_separator = false;
            continue;
        }

        if (!previous_separator && used + 1U < hostname_size)
        {
            hostname[used++] = '-';
            previous_separator = true;
        }
    }

    while (used > 0 && hostname[used - 1U] == '-')
    {
        used--;
    }

    if (used == 0)
    {
        snprintf(hostname, hostname_size, "%s", MANAGEMENT_DEVICE_DEFAULT_HOSTNAME);
        return;
    }

    hostname[used] = '\0';
}

static esp_log_level_t management_device_log_level_from_record(int32_t value)
{
    switch (value)
    {
    case ESP_LOG_ERROR:
        return ESP_LOG_ERROR;
    case ESP_LOG_WARN:
        return ESP_LOG_WARN;
    case ESP_LOG_INFO:
        return ESP_LOG_INFO;
    case ESP_LOG_DEBUG:
        return ESP_LOG_DEBUG;
    case ESP_LOG_VERBOSE:
        return ESP_LOG_VERBOSE;
    default:
        return ESP_LOG_INFO;
    }
}

static void management_device_apply_log_level_locked(void)
{
    const esp_log_level_t level =
        management_device_log_level_from_record(management_device_config_record.log_level);
    esp_log_level_set("*", level);
#if CONFIG_LOG_MASTER_LEVEL
    esp_log_set_level_master(level);
#endif
}

static void management_device_config_set_defaults_locked(void)
{
    memset(&management_device_config_record, 0, sizeof(management_device_config_record));
    management_device_config_record.version = MANAGEMENT_DEVICE_CONFIG_VERSION;
    management_device_config_record.log_level = ESP_LOG_INFO;
    snprintf(management_device_config_record.device_name,
             sizeof(management_device_config_record.device_name), "%s",
             MANAGEMENT_DEVICE_DEFAULT_NAME);
}

static void management_device_config_load(void)
{
    management_device_config_set_defaults_locked();

    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(MANAGEMENT_DEVICE_CONFIG_NAMESPACE, NVS_READONLY,
                                &handle);
    if (result != ESP_OK)
    {
        management_device_config_initialized = true;
        management_device_apply_log_level_locked();
        return;
    }

    ManagementDeviceConfigRecord stored = {0};
    size_t stored_size = sizeof(stored);
    result = nvs_get_blob(handle, MANAGEMENT_DEVICE_CONFIG_KEY, &stored, &stored_size);
    nvs_close(handle);
    if (result == ESP_OK && stored_size == sizeof(stored) &&
        stored.version == MANAGEMENT_DEVICE_CONFIG_VERSION)
    {
        char trimmed_name[MANAGEMENT_DEVICE_NAME_MAX_LENGTH + 1U];
        if (management_device_name_copy_trimmed(stored.device_name, trimmed_name,
                                                sizeof(trimmed_name)))
        {
            snprintf(management_device_config_record.device_name,
                     sizeof(management_device_config_record.device_name), "%s",
                     trimmed_name);
        }
        else
        {
            ESP_LOGW(TAG, "Stored device name was invalid; using the default name");
        }
        management_device_config_record.log_level =
            (int32_t)management_device_log_level_from_record(stored.log_level);
    }
    else if (result != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Unable to load stored device configuration: %s",
                 esp_err_to_name(result));
    }

    management_device_apply_log_level_locked();
    management_device_config_initialized = true;
}

static esp_err_t management_device_config_write_locked(const ManagementDeviceConfigRecord *record)
{
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(MANAGEMENT_DEVICE_CONFIG_NAMESPACE, NVS_READWRITE,
                                &handle);
    if (result != ESP_OK)
    {
        return result;
    }

    result = nvs_set_blob(handle, MANAGEMENT_DEVICE_CONFIG_KEY, record, sizeof(*record));
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    return result;
}

void management_device_config_initialize(void)
{
    if (!management_device_config_initialized)
    {
        management_device_config_load();
    }
    else
    {
        management_device_apply_log_level_locked();
    }
}

void management_device_config_snapshot(ManagementDeviceConfigSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    if (!management_device_config_initialized)
    {
        management_device_config_load();
    }

    taskENTER_CRITICAL(&management_device_config_lock);
    snprintf(snapshot->device_name, sizeof(snapshot->device_name), "%s",
             management_device_config_record.device_name);
    management_device_build_hostname(management_device_config_record.device_name,
                                     snapshot->hostname, sizeof(snapshot->hostname));
    snapshot->log_level =
        management_device_log_level_from_record(management_device_config_record.log_level);
    taskEXIT_CRITICAL(&management_device_config_lock);
}

esp_err_t management_device_config_save(const char *device_name, esp_log_level_t log_level)
{
    char trimmed_name[MANAGEMENT_DEVICE_NAME_MAX_LENGTH + 1U];
    if (!management_device_name_copy_trimmed(device_name, trimmed_name,
                                             sizeof(trimmed_name)))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (log_level < ESP_LOG_ERROR || log_level > ESP_LOG_VERBOSE)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ManagementDeviceConfigRecord record = {
        .version = MANAGEMENT_DEVICE_CONFIG_VERSION,
        .log_level = (int32_t)log_level,
    };
    snprintf(record.device_name, sizeof(record.device_name), "%s", trimmed_name);

    const esp_err_t result = management_device_config_write_locked(&record);
    if (result == ESP_OK)
    {
        taskENTER_CRITICAL(&management_device_config_lock);
        management_device_config_record = record;
        management_device_config_initialized = true;
        taskEXIT_CRITICAL(&management_device_config_lock);
        management_device_apply_log_level_locked();
    }

    return result;
}

const char *management_device_config_log_level_name(esp_log_level_t level)
{
    switch (level)
    {
    case ESP_LOG_ERROR:
        return "error";
    case ESP_LOG_WARN:
        return "warning";
    case ESP_LOG_DEBUG:
        return "debug";
    case ESP_LOG_VERBOSE:
        return "verbose";
    case ESP_LOG_INFO:
    default:
        return "info";
    }
}

bool management_device_config_parse_log_level(const char *value,
                                              esp_log_level_t *level)
{
    if (value == NULL || level == NULL)
    {
        return false;
    }

    if (strcasecmp(value, "error") == 0)
    {
        *level = ESP_LOG_ERROR;
        return true;
    }
    if (strcasecmp(value, "warning") == 0)
    {
        *level = ESP_LOG_WARN;
        return true;
    }
    if (strcasecmp(value, "info") == 0)
    {
        *level = ESP_LOG_INFO;
        return true;
    }
    if (strcasecmp(value, "debug") == 0)
    {
        *level = ESP_LOG_DEBUG;
        return true;
    }
    if (strcasecmp(value, "verbose") == 0)
    {
        *level = ESP_LOG_VERBOSE;
        return true;
    }

    return false;
}
