/** @file management-device-routes.c @brief Handle authenticated device identity and log-level requests. @see management-device-routes.h, management-device-config.h, management-session.h, management-http.h */
#include "management-device-routes.h"

#include <string.h>

#include "management-device-config.h"
#include "management-http.h"
#include "management-session.h"
#include "wifi-provisioning.h"

#include "esp_err.h"
#include "esp_log.h"
#include "mbedtls/platform_util.h"

#define TAG "nut-management"

esp_err_t management_device_config_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(
            request, "403 Forbidden",
            "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char device_name[MANAGEMENT_DEVICE_NAME_MAX_LENGTH + 1U] = {0};
    char log_level_name[16] = {0};
    const esp_err_t form_result = management_read_form_body(request, body, sizeof(body));
    const bool fields_present = form_result == ESP_OK &&
                                management_form_value(body, "device_name", device_name,
                                                      sizeof(device_name)) &&
                                management_form_value(body, "log_level", log_level_name,
                                                      sizeof(log_level_name));
    mbedtls_platform_zeroize(body, sizeof(body));
    if (!fields_present)
    {
        mbedtls_platform_zeroize(device_name, sizeof(device_name));
        mbedtls_platform_zeroize(log_level_name, sizeof(log_level_name));
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"A device name and log level are required.\"}");
    }

    esp_log_level_t log_level = ESP_LOG_INFO;
    if (!management_device_config_parse_log_level(log_level_name, &log_level))
    {
        mbedtls_platform_zeroize(device_name, sizeof(device_name));
        mbedtls_platform_zeroize(log_level_name, sizeof(log_level_name));
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"Choose Error, Warning, Info, Debug, or Verbose.\"}");
    }

    const esp_err_t save_result = management_device_config_save(device_name, log_level);
    mbedtls_platform_zeroize(device_name, sizeof(device_name));
    mbedtls_platform_zeroize(log_level_name, sizeof(log_level_name));
    if (save_result == ESP_ERR_INVALID_ARG)
    {
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"Use a trimmed, nonempty device name without control characters.\"}");
    }
    if (save_result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to save device configuration: %s",
                 esp_err_to_name(save_result));
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to save the device configuration.\"}");
    }

    ManagementDeviceConfigSnapshot snapshot;
    management_device_config_snapshot(&snapshot);
    const esp_err_t hostname_result =
        wifi_provisioning_set_station_hostname(snapshot.hostname);
    if (hostname_result != ESP_OK)
    {
        ESP_LOGW(TAG, "Unable to apply the station hostname now: %s",
                 esp_err_to_name(hostname_result));
    }

    char response[384];
    const int response_length = snprintf(
        response, sizeof(response),
        "{\"message\":\"Device settings saved. The hostname will be advertised on the next Wi-Fi reconnect or reboot.\","
        "\"device_name\":\"%s\",\"hostname\":\"%s\",\"log_level\":\"%s\"}",
        snapshot.device_name, snapshot.hostname,
        management_device_config_log_level_name(snapshot.log_level));
    esp_err_t send_result;
    if (response_length < 0 || response_length >= (int)sizeof(response))
    {
        send_result = management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"The device settings were saved but the response could not be prepared.\"}");
    }
    else
    {
        send_result = management_send_json(request, "200 OK", response);
    }
    mbedtls_platform_zeroize(response, sizeof(response));
    mbedtls_platform_zeroize(&snapshot, sizeof(snapshot));
    return send_result;
}
