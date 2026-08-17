/** @file management-time-routes.c @brief Handle authenticated time-configuration requests. @see management-time-routes.h, time_config.h, management-session.h, management-http.h */
#include "management-time-routes.h"

#include <stdbool.h>
#include <string.h>

#include "management-http.h"
#include "management-session.h"

#include "esp_err.h"
#include "esp_log.h"
#include "mbedtls/platform_util.h"
#include "time_config.h"

#define TAG "nut-management"

esp_err_t management_time_config_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden",
                                    "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char action[16] = {0};
    const esp_err_t form_result = management_read_form_body(request, body, sizeof(body));
    if (form_result != ESP_OK ||
        !management_form_value(body, "action", action, sizeof(action)))
    {
        mbedtls_platform_zeroize(body, sizeof(body));
        return management_send_json(request, "400 Bad Request",
                                    "{\"error\":\"A valid time action is required.\"}");
    }

    esp_err_t result = ESP_ERR_INVALID_ARG;
    if (strcmp(action, "configure") == 0)
    {
        char ntp_enabled[6] = {0};
        char ntp_server[TIME_CONFIG_NTP_SERVER_MAX_LENGTH + 1] = {0};
        char timezone[TIME_CONFIG_TIMEZONE_MAX_LENGTH + 1] = {0};
        const bool fields_present =
            management_form_value(body, "ntp_enabled", ntp_enabled,
                                  sizeof(ntp_enabled)) &&
            management_form_value(body, "ntp_server", ntp_server,
                                  sizeof(ntp_server)) &&
            management_form_value(body, "timezone", timezone,
                                  sizeof(timezone));
        const bool enabled_value_valid = strcmp(ntp_enabled, "true") == 0 ||
                                         strcmp(ntp_enabled, "false") == 0;
        if (!fields_present || !enabled_value_valid)
        {
            mbedtls_platform_zeroize(body, sizeof(body));
            return management_send_json(request, "400 Bad Request",
                                        "{\"error\":\"The time configuration is invalid.\"}");
        }
        result = time_config_update(strcmp(ntp_enabled, "true") == 0,
                                    ntp_server, timezone);
        mbedtls_platform_zeroize(ntp_server, sizeof(ntp_server));
        mbedtls_platform_zeroize(timezone, sizeof(timezone));
        if (result == ESP_ERR_INVALID_ARG)
        {
            mbedtls_platform_zeroize(body, sizeof(body));
            return management_send_json(
                request, "400 Bad Request",
                "{\"error\":\"Use a valid NTP hostname and supported IANA time zone.\"}");
        }
        if (result == ESP_OK)
        {
            mbedtls_platform_zeroize(body, sizeof(body));
            return management_send_json(
                request, "200 OK",
                "{\"message\":\"Time configuration saved.\"}");
        }
    }
    else if (strcmp(action, "manual") == 0)
    {
        char local_datetime[17] = {0};
        if (!management_form_value(body, "local_datetime", local_datetime,
                                   sizeof(local_datetime)))
        {
            mbedtls_platform_zeroize(body, sizeof(body));
            return management_send_json(request, "400 Bad Request",
                                        "{\"error\":\"A local date and time are required.\"}");
        }
        result = time_config_set_manual(local_datetime);
        mbedtls_platform_zeroize(local_datetime, sizeof(local_datetime));
        if (result == ESP_ERR_INVALID_ARG)
        {
            mbedtls_platform_zeroize(body, sizeof(body));
            return management_send_json(
                request, "400 Bad Request",
                "{\"error\":\"Use a valid date and time from 2024 through 2099 in the configured time zone.\"}");
        }
        if (result == ESP_OK)
        {
            mbedtls_platform_zeroize(body, sizeof(body));
            return management_send_json(
                request, "200 OK",
                "{\"message\":\"Device date and time set manually.\"}");
        }
    }
    else if (strcmp(action, "sync") == 0)
    {
        result = time_config_request_sync();
        if (result == ESP_ERR_INVALID_STATE)
        {
            mbedtls_platform_zeroize(body, sizeof(body));
            return management_send_json(
                request, "409 Conflict",
                "{\"error\":\"Enable NTP before requesting synchronization.\"}");
        }
        if (result == ESP_OK)
        {
            mbedtls_platform_zeroize(body, sizeof(body));
            return management_send_json(
                request, "202 Accepted",
                "{\"message\":\"NTP synchronization requested.\"}");
        }
    }
    else
    {
        mbedtls_platform_zeroize(body, sizeof(body));
        return management_send_json(request, "400 Bad Request",
                                    "{\"error\":\"Unknown time action.\"}");
    }

    mbedtls_platform_zeroize(body, sizeof(body));
    ESP_LOGE(TAG, "Time action '%s' failed: %s", action, esp_err_to_name(result));
    return management_send_json(request, "500 Internal Server Error",
                                "{\"error\":\"Unable to apply the time configuration.\"}");
}
