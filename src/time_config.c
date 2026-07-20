#include "time_config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include "nvs.h"

#define TAG "nut-time"

#define TIME_CONFIG_NAMESPACE "management"
#define TIME_CONFIG_NVS_KEY "time-cfg"
#define TIME_CONFIG_VERSION 1U
#define TIME_CONFIG_DEFAULT_NTP_SERVER "pool.ntp.org"
#define TIME_CONFIG_DEFAULT_TIMEZONE "America/Los_Angeles"
#define TIME_CONFIG_VALID_EPOCH 1704067200LL

_Static_assert(sizeof(TIME_CONFIG_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "Time-config NVS namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(TIME_CONFIG_NVS_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "Time-config NVS key exceeds the ESP-IDF limit");

typedef struct
{
    const char *iana;
    const char *posix;
} TimezoneMapping;

typedef struct
{
    uint32_t version;
    uint8_t ntp_enabled;
    uint8_t reserved[3];
    char ntp_server[TIME_CONFIG_NTP_SERVER_MAX_LENGTH + 1];
    char timezone[TIME_CONFIG_TIMEZONE_MAX_LENGTH + 1];
} StoredTimeConfiguration;

typedef enum
{
    TIME_SOURCE_UNAVAILABLE,
    TIME_SOURCE_RETAINED,
    TIME_SOURCE_MANUAL,
    TIME_SOURCE_NTP,
} TimeSource;

static const TimezoneMapping timezones[] = {
    {"UTC", "UTC0"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2"},
    {"America/Denver", "MST7MDT,M3.2.0/2,M11.1.0/2"},
    {"America/Phoenix", "MST7"},
    {"America/Chicago", "CST6CDT,M3.2.0/2,M11.1.0/2"},
    {"America/New_York", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {"America/Anchorage", "AKST9AKDT,M3.2.0/2,M11.1.0/2"},
    {"Pacific/Honolulu", "HST10"},
};

static StoredTimeConfiguration current_configuration;
/* lwIP retains the initial server-name pointer until SNTP is reconfigured. */
static char active_sntp_server[TIME_CONFIG_NTP_SERVER_MAX_LENGTH + 1];
static bool configuration_loaded;
static bool sntp_initialized;
static bool synchronization_pending;
static bool ntp_synchronized;
static TimeSource time_source;
static portMUX_TYPE time_state_lock = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t time_operation_mutex;

static const TimezoneMapping *time_config_find_timezone(const char *iana_timezone)
{
    if (iana_timezone == NULL)
    {
        return NULL;
    }
    for (size_t index = 0; index < sizeof(timezones) / sizeof(timezones[0]); index++)
    {
        if (strcmp(timezones[index].iana, iana_timezone) == 0)
        {
            return &timezones[index];
        }
    }
    return NULL;
}

static bool time_config_ntp_server_is_valid(const char *server)
{
    if (server == NULL)
    {
        return false;
    }
    const size_t length = strlen(server);
    if (length == 0 || length > TIME_CONFIG_NTP_SERVER_MAX_LENGTH ||
        server[0] == '.' || server[0] == '-' ||
        server[length - 1] == '.' || server[length - 1] == '-')
    {
        return false;
    }
    for (size_t index = 0; index < length; index++)
    {
        const char character = server[index];
        if (!((character >= 'a' && character <= 'z') ||
              (character >= 'A' && character <= 'Z') ||
              (character >= '0' && character <= '9') ||
              character == '.' || character == '-'))
        {
            return false;
        }
    }
    return true;
}

static StoredTimeConfiguration time_config_defaults(void)
{
    StoredTimeConfiguration configuration = {
        .version = TIME_CONFIG_VERSION,
        .ntp_enabled = 1,
    };
    snprintf(configuration.ntp_server, sizeof(configuration.ntp_server), "%s",
             TIME_CONFIG_DEFAULT_NTP_SERVER);
    snprintf(configuration.timezone, sizeof(configuration.timezone), "%s",
             TIME_CONFIG_DEFAULT_TIMEZONE);
    return configuration;
}

static StoredTimeConfiguration time_config_load(void)
{
    StoredTimeConfiguration configuration = time_config_defaults();
    nvs_handle_t handle = 0;
    if (nvs_open(TIME_CONFIG_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
    {
        return configuration;
    }

    StoredTimeConfiguration stored = {0};
    size_t stored_length = sizeof(stored);
    const esp_err_t result = nvs_get_blob(handle, TIME_CONFIG_NVS_KEY,
                                          &stored, &stored_length);
    nvs_close(handle);
    if (result == ESP_OK && stored_length == sizeof(stored) &&
        stored.version == TIME_CONFIG_VERSION &&
        stored.ntp_enabled <= 1 &&
        time_config_ntp_server_is_valid(stored.ntp_server) &&
        time_config_find_timezone(stored.timezone) != NULL)
    {
        configuration = stored;
    }
    else if (result == ESP_OK)
    {
        ESP_LOGW(TAG, "Ignoring invalid stored time-configuration contents");
    }
    else if (result != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Unable to load stored time configuration: %s",
                 esp_err_to_name(result));
    }
    return configuration;
}

static esp_err_t time_config_store(const StoredTimeConfiguration *configuration)
{
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(TIME_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, TIME_CONFIG_NVS_KEY,
                              configuration, sizeof(*configuration));
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    if (handle != 0)
    {
        nvs_close(handle);
    }
    return result;
}

static esp_err_t time_config_apply_timezone(const char *iana_timezone)
{
    const TimezoneMapping *mapping = time_config_find_timezone(iana_timezone);
    if (mapping == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (setenv("TZ", mapping->posix, 1) != 0)
    {
        return ESP_FAIL;
    }
    tzset();
    return ESP_OK;
}

static void time_config_sync_callback(struct timeval *value)
{
    const bool valid = value != NULL && value->tv_sec >= TIME_CONFIG_VALID_EPOCH;
    taskENTER_CRITICAL(&time_state_lock);
    synchronization_pending = false;
    ntp_synchronized = valid;
    if (valid)
    {
        time_source = TIME_SOURCE_NTP;
    }
    taskEXIT_CRITICAL(&time_state_lock);
    if (valid)
    {
        ESP_LOGI(TAG, "System time synchronized through SNTP");
    }
    else
    {
        ESP_LOGW(TAG, "SNTP returned an invalid system time");
    }
}

static esp_err_t time_config_apply_sntp(const StoredTimeConfiguration *configuration)
{
    if (sntp_initialized)
    {
        esp_netif_sntp_deinit();
        sntp_initialized = false;
    }

    taskENTER_CRITICAL(&time_state_lock);
    synchronization_pending = configuration->ntp_enabled != 0;
    taskEXIT_CRITICAL(&time_state_lock);
    if (configuration->ntp_enabled == 0)
    {
        active_sntp_server[0] = '\0';
        return ESP_OK;
    }

    /*
     * ESP_NETIF_SNTP_DEFAULT_CONFIG does not copy its server string during
     * initial setup. Keep the backing storage alive for the complete SNTP
     * lifetime; configuration may point to a caller's stack record.
     */
    snprintf(active_sntp_server, sizeof(active_sntp_server), "%s",
             configuration->ntp_server);
    esp_sntp_config_t sntp_configuration =
        ESP_NETIF_SNTP_DEFAULT_CONFIG(active_sntp_server);
    sntp_configuration.wait_for_sync = false;
    sntp_configuration.renew_servers_after_new_IP = true;
    sntp_configuration.sync_cb = time_config_sync_callback;
    const esp_err_t result = esp_netif_sntp_init(&sntp_configuration);
    if (result == ESP_OK)
    {
        sntp_initialized = true;
        ESP_LOGI(TAG, "SNTP started with server '%s'", active_sntp_server);
    }
    else
    {
        taskENTER_CRITICAL(&time_state_lock);
        synchronization_pending = false;
        taskEXIT_CRITICAL(&time_state_lock);
    }
    return result;
}

esp_err_t time_config_start(void)
{
    if (time_operation_mutex == NULL)
    {
        time_operation_mutex = xSemaphoreCreateMutex();
        if (time_operation_mutex == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }
    xSemaphoreTake(time_operation_mutex, portMAX_DELAY);

    StoredTimeConfiguration configuration;
    taskENTER_CRITICAL(&time_state_lock);
    const bool loaded = configuration_loaded;
    configuration = current_configuration;
    taskEXIT_CRITICAL(&time_state_lock);
    if (!loaded)
    {
        configuration = time_config_load();
        taskENTER_CRITICAL(&time_state_lock);
        current_configuration = configuration;
        configuration_loaded = true;
        taskEXIT_CRITICAL(&time_state_lock);
    }

    esp_err_t result = time_config_apply_timezone(configuration.timezone);
    if (result != ESP_OK)
    {
        xSemaphoreGive(time_operation_mutex);
        return result;
    }

    const time_t now = time(NULL);
    taskENTER_CRITICAL(&time_state_lock);
    if (now >= TIME_CONFIG_VALID_EPOCH && time_source == TIME_SOURCE_UNAVAILABLE)
    {
        time_source = TIME_SOURCE_RETAINED;
    }
    taskEXIT_CRITICAL(&time_state_lock);
    result = time_config_apply_sntp(&configuration);
    xSemaphoreGive(time_operation_mutex);
    return result;
}

esp_err_t time_config_update(bool ntp_enabled, const char *ntp_server,
                             const char *iana_timezone)
{
    if (!time_config_ntp_server_is_valid(ntp_server) ||
        time_config_find_timezone(iana_timezone) == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (time_operation_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    StoredTimeConfiguration configuration = {
        .version = TIME_CONFIG_VERSION,
        .ntp_enabled = ntp_enabled ? 1 : 0,
    };
    snprintf(configuration.ntp_server, sizeof(configuration.ntp_server), "%s",
             ntp_server);
    snprintf(configuration.timezone, sizeof(configuration.timezone), "%s",
             iana_timezone);

    xSemaphoreTake(time_operation_mutex, portMAX_DELAY);
    esp_err_t result = time_config_store(&configuration);
    if (result != ESP_OK)
    {
        xSemaphoreGive(time_operation_mutex);
        return result;
    }
    result = time_config_apply_timezone(configuration.timezone);
    if (result != ESP_OK)
    {
        xSemaphoreGive(time_operation_mutex);
        return result;
    }
    taskENTER_CRITICAL(&time_state_lock);
    current_configuration = configuration;
    configuration_loaded = true;
    taskEXIT_CRITICAL(&time_state_lock);
    result = time_config_apply_sntp(&configuration);
    xSemaphoreGive(time_operation_mutex);
    return result;
}

static bool time_config_parse_local_datetime(const char *value, struct tm *parsed)
{
    if (value == NULL || parsed == NULL || strlen(value) != 16 ||
        value[4] != '-' || value[7] != '-' || value[10] != 'T' || value[13] != ':')
    {
        return false;
    }
    for (size_t index = 0; index < 16; index++)
    {
        if (index == 4 || index == 7 || index == 10 || index == 13)
        {
            continue;
        }
        if (value[index] < '0' || value[index] > '9')
        {
            return false;
        }
    }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    if (sscanf(value, "%4d-%2d-%2dT%2d:%2d",
               &year, &month, &day, &hour, &minute) != 5 ||
        year < 2024 || year > 2099 || month < 1 || month > 12 ||
        day < 1 || day > 31 || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59)
    {
        return false;
    }

    *parsed = (struct tm){
        .tm_year = year - 1900,
        .tm_mon = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = minute,
        .tm_sec = 0,
        .tm_isdst = -1,
    };
    return true;
}

esp_err_t time_config_set_manual(const char *local_datetime)
{
    if (time_operation_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreTake(time_operation_mutex, portMAX_DELAY);
    struct tm requested = {0};
    if (!time_config_parse_local_datetime(local_datetime, &requested))
    {
        xSemaphoreGive(time_operation_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    const struct tm original = requested;
    const time_t epoch = mktime(&requested);
    if (epoch < TIME_CONFIG_VALID_EPOCH)
    {
        xSemaphoreGive(time_operation_mutex);
        return ESP_ERR_INVALID_ARG;
    }
    struct tm round_trip = {0};
    localtime_r(&epoch, &round_trip);
    if (round_trip.tm_year != original.tm_year ||
        round_trip.tm_mon != original.tm_mon ||
        round_trip.tm_mday != original.tm_mday ||
        round_trip.tm_hour != original.tm_hour ||
        round_trip.tm_min != original.tm_min)
    {
        xSemaphoreGive(time_operation_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    const struct timeval value = {.tv_sec = epoch, .tv_usec = 0};
    if (settimeofday(&value, NULL) != 0)
    {
        xSemaphoreGive(time_operation_mutex);
        return ESP_FAIL;
    }
    char timezone[TIME_CONFIG_TIMEZONE_MAX_LENGTH + 1];
    taskENTER_CRITICAL(&time_state_lock);
    ntp_synchronized = false;
    time_source = TIME_SOURCE_MANUAL;
    memcpy(timezone, current_configuration.timezone, sizeof(timezone));
    timezone[sizeof(timezone) - 1] = '\0';
    taskEXIT_CRITICAL(&time_state_lock);
    ESP_LOGI(TAG, "System time set manually in time zone '%s'",
             timezone);
    xSemaphoreGive(time_operation_mutex);
    return ESP_OK;
}

esp_err_t time_config_request_sync(void)
{
    if (time_operation_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreTake(time_operation_mutex, portMAX_DELAY);
    taskENTER_CRITICAL(&time_state_lock);
    const bool enabled = configuration_loaded &&
                         current_configuration.ntp_enabled != 0;
    taskEXIT_CRITICAL(&time_state_lock);
    if (!enabled || !sntp_initialized)
    {
        xSemaphoreGive(time_operation_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    taskENTER_CRITICAL(&time_state_lock);
    synchronization_pending = true;
    taskEXIT_CRITICAL(&time_state_lock);
    const esp_err_t result = esp_netif_sntp_start();
    if (result != ESP_OK)
    {
        taskENTER_CRITICAL(&time_state_lock);
        synchronization_pending = false;
        taskEXIT_CRITICAL(&time_state_lock);
    }
    xSemaphoreGive(time_operation_mutex);
    return result;
}

static const char *time_config_source_name(TimeSource source)
{
    switch (source)
    {
        case TIME_SOURCE_NTP:
            return "ntp";
        case TIME_SOURCE_MANUAL:
            return "manual";
        case TIME_SOURCE_RETAINED:
            return "retained";
        default:
            return "unavailable";
    }
}

void time_config_get_status(TimeConfigStatus *status)
{
    if (status == NULL)
    {
        return;
    }
    const bool operation_locked = time_operation_mutex != NULL &&
                                  xSemaphoreTake(time_operation_mutex,
                                                 portMAX_DELAY) == pdTRUE;
    memset(status, 0, sizeof(*status));
    const time_t now = time(NULL);
    TimeSource source;
    StoredTimeConfiguration configuration = time_config_defaults();
    bool loaded;
    taskENTER_CRITICAL(&time_state_lock);
    source = time_source;
    status->ntp_synchronized = ntp_synchronized;
    status->synchronization_pending = synchronization_pending;
    loaded = configuration_loaded;
    if (loaded)
    {
        configuration = current_configuration;
    }
    taskEXIT_CRITICAL(&time_state_lock);

    status->available = now >= TIME_CONFIG_VALID_EPOCH &&
                        source != TIME_SOURCE_UNAVAILABLE;
    status->ntp_enabled = loaded && configuration.ntp_enabled != 0;
    snprintf(status->source, sizeof(status->source), "%s",
             time_config_source_name(source));
    snprintf(status->timezone, sizeof(status->timezone), "%s",
             configuration.timezone);
    snprintf(status->ntp_server, sizeof(status->ntp_server), "%s",
             configuration.ntp_server);
    if (!status->available)
    {
        if (operation_locked)
        {
            xSemaphoreGive(time_operation_mutex);
        }
        return;
    }

    struct tm utc = {0};
    struct tm local = {0};
    gmtime_r(&now, &utc);
    localtime_r(&now, &local);
    strftime(status->utc, sizeof(status->utc), "%Y-%m-%dT%H:%M:%SZ", &utc);
    strftime(status->local, sizeof(status->local), "%Y-%m-%dT%H:%M:%S%z", &local);
    if (operation_locked)
    {
        xSemaphoreGive(time_operation_mutex);
    }
}
