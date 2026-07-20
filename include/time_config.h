#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#define TIME_CONFIG_NTP_SERVER_MAX_LENGTH 63
#define TIME_CONFIG_TIMEZONE_MAX_LENGTH 39
#define TIME_CONFIG_TIMESTAMP_MAX_LENGTH 39

typedef struct
{
    bool available;
    bool ntp_enabled;
    bool ntp_synchronized;
    bool synchronization_pending;
    char source[12];
    char utc[TIME_CONFIG_TIMESTAMP_MAX_LENGTH + 1];
    char local[TIME_CONFIG_TIMESTAMP_MAX_LENGTH + 1];
    char timezone[TIME_CONFIG_TIMEZONE_MAX_LENGTH + 1];
    char ntp_server[TIME_CONFIG_NTP_SERVER_MAX_LENGTH + 1];
} TimeConfigStatus;

/**
 * Load persisted time settings, apply the configured time zone, and start SNTP
 * after station Wi-Fi has received an IPv4 address.
 */
esp_err_t time_config_start(void);

/** Persist and apply the NTP and IANA time-zone configuration. */
esp_err_t time_config_update(bool ntp_enabled, const char *ntp_server,
                             const char *iana_timezone);

/** Set the clock from a YYYY-MM-DDTHH:MM value in the configured time zone. */
esp_err_t time_config_set_manual(const char *local_datetime);

/** Request an immediate SNTP synchronization using the persisted server. */
esp_err_t time_config_request_sync(void);

/** Return current time, configuration, source, and synchronization state. */
void time_config_get_status(TimeConfigStatus *status);
