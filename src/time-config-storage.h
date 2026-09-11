#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define TIME_CONFIG_STORAGE_NTP_SERVER_MAX_LENGTH 63
#define TIME_CONFIG_STORAGE_TIMEZONE_MAX_LENGTH 39
#define TIME_CONFIG_STORAGE_VERSION 1U

typedef struct
{
    uint32_t version;
    uint8_t ntp_enabled;
    uint8_t reserved[3];
    char ntp_server[TIME_CONFIG_STORAGE_NTP_SERVER_MAX_LENGTH + 1];
    char timezone[TIME_CONFIG_STORAGE_TIMEZONE_MAX_LENGTH + 1];
} TimeConfigStorage;

TimeConfigStorage time_config_storage_defaults(void);
TimeConfigStorage time_config_storage_load(void);
esp_err_t time_config_storage_store(const TimeConfigStorage *configuration);
bool time_config_storage_ntp_server_is_valid(const char *server);
bool time_config_storage_timezone_is_valid(const char *iana_timezone);
esp_err_t time_config_storage_apply_timezone(const char *iana_timezone);
