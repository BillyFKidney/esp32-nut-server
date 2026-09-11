/** @file time-config-storage.c @brief Persist time settings and apply supported time zones. */
#include "time-config-storage.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "nvs.h"

#define TAG "nut-time"
#define TIME_CONFIG_NAMESPACE "management"
#define TIME_CONFIG_NVS_KEY "time-cfg"
#define TIME_CONFIG_DEFAULT_NTP_SERVER "pool.ntp.org"
#define TIME_CONFIG_DEFAULT_TIMEZONE "America/Los_Angeles"

typedef struct { const char *iana; const char *posix; } TimezoneMapping;

static const TimezoneMapping timezones[] = {
    {"UTC", "UTC0"}, {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2"},
    {"America/Denver", "MST7MDT,M3.2.0/2,M11.1.0/2"}, {"America/Phoenix", "MST7"},
    {"America/Chicago", "CST6CDT,M3.2.0/2,M11.1.0/2"}, {"America/New_York", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {"America/Anchorage", "AKST9AKDT,M3.2.0/2,M11.1.0/2"}, {"Pacific/Honolulu", "HST10"},
};

_Static_assert(sizeof(TIME_CONFIG_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE, "Time-config namespace too long");
_Static_assert(sizeof(TIME_CONFIG_NVS_KEY) <= NVS_KEY_NAME_MAX_SIZE, "Time-config key too long");

static const TimezoneMapping *find_timezone(const char *iana_timezone)
{
    if (iana_timezone == NULL) return NULL;
    for (size_t index = 0; index < sizeof(timezones) / sizeof(timezones[0]); index++)
        if (strcmp(timezones[index].iana, iana_timezone) == 0) return &timezones[index];
    return NULL;
}

bool time_config_storage_ntp_server_is_valid(const char *server)
{
    if (server == NULL) return false;
    const size_t length = strlen(server);
    if (length == 0 || length > TIME_CONFIG_STORAGE_NTP_SERVER_MAX_LENGTH ||
        server[0] == '.' || server[0] == '-' || server[length - 1] == '.' || server[length - 1] == '-') return false;
    for (size_t index = 0; index < length; index++) {
        const char character = server[index];
        if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
              (character >= '0' && character <= '9') || character == '.' || character == '-')) return false;
    }
    return true;
}

bool time_config_storage_timezone_is_valid(const char *iana_timezone)
{
    return find_timezone(iana_timezone) != NULL;
}

TimeConfigStorage time_config_storage_defaults(void)
{
    TimeConfigStorage configuration = {
        .version = TIME_CONFIG_STORAGE_VERSION,
        .ntp_enabled = 1,
    };
    snprintf(configuration.ntp_server, sizeof(configuration.ntp_server), "%s", TIME_CONFIG_DEFAULT_NTP_SERVER);
    snprintf(configuration.timezone, sizeof(configuration.timezone), "%s", TIME_CONFIG_DEFAULT_TIMEZONE);
    return configuration;
}

TimeConfigStorage time_config_storage_load(void)
{
    TimeConfigStorage configuration = time_config_storage_defaults();
    nvs_handle_t handle = 0;
    if (nvs_open(TIME_CONFIG_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return configuration;
    TimeConfigStorage stored = {0}; size_t stored_length = sizeof(stored);
    const esp_err_t result = nvs_get_blob(handle, TIME_CONFIG_NVS_KEY, &stored, &stored_length);
    nvs_close(handle);
    if (result == ESP_OK && stored_length == sizeof(stored) &&
        stored.version == TIME_CONFIG_STORAGE_VERSION &&
        stored.ntp_enabled <= 1 && time_config_storage_ntp_server_is_valid(stored.ntp_server) && find_timezone(stored.timezone) != NULL) configuration = stored;
    else if (result == ESP_OK) ESP_LOGW(TAG, "Ignoring invalid stored time-configuration contents");
    else if (result != ESP_ERR_NVS_NOT_FOUND) ESP_LOGW(TAG, "Unable to load stored time configuration: %s", esp_err_to_name(result));
    return configuration;
}

esp_err_t time_config_storage_store(const TimeConfigStorage *configuration)
{
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(TIME_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (result == ESP_OK) result = nvs_set_blob(handle, TIME_CONFIG_NVS_KEY, configuration, sizeof(*configuration));
    if (result == ESP_OK) result = nvs_commit(handle);
    if (handle != 0) nvs_close(handle);
    return result;
}

esp_err_t time_config_storage_apply_timezone(const char *iana_timezone)
{
    const TimezoneMapping *mapping = find_timezone(iana_timezone);
    if (mapping == NULL) return ESP_ERR_INVALID_ARG;
    if (setenv("TZ", mapping->posix, 1) != 0) return ESP_FAIL;
    tzset();
    return ESP_OK;
}
