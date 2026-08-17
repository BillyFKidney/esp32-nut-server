#include "management-wifi-routes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "management-authorization.h"
#include "management-http.h"
#include "management-session.h"
#include "wifi-provisioning.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "mbedtls/platform_util.h"

#define TAG "nut-management"
#define MANAGEMENT_WIFI_SCAN_RESPONSE_SIZE 4200U

static const char *management_wifi_security_name(uint8_t authmode)
{
    switch ((wifi_auth_mode_t)authmode)
    {
    case WIFI_AUTH_OPEN:
        return "Open";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK:
        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2/WPA3";
    case WIFI_AUTH_OWE:
        return "OWE";
    case WIFI_AUTH_ENTERPRISE:
    case WIFI_AUTH_WPA3_ENTERPRISE:
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
    case WIFI_AUTH_WPA_ENTERPRISE:
    case WIFI_AUTH_WPA3_ENT_192:
        return "Enterprise";
    default:
        return "Unknown";
    }
}

esp_err_t management_wifi_scan_handler(httpd_req_t *request)
{
    if (!management_require_session(request, true))
    {
        return ESP_OK;
    }

    WifiManagementScanResults results;
    const esp_err_t scan_result = wifi_management_scan(&results);
    if (scan_result == ESP_ERR_INVALID_STATE)
    {
        return management_send_json(
            request, "409 Conflict",
            "{\"error\":\"Wi-Fi must be connected before scanning for networks.\"}");
    }
    if (scan_result == ESP_ERR_TIMEOUT || scan_result == ESP_ERR_WIFI_STATE)
    {
        return management_send_json(
            request, "503 Service Unavailable",
            "{\"error\":\"Wi-Fi is busy. Wait a moment and scan again.\"}");
    }
    if (scan_result != ESP_OK)
    {
        ESP_LOGW(TAG, "Unable to scan Wi-Fi networks: %s", esp_err_to_name(scan_result));
        return management_send_json(
            request, "503 Service Unavailable",
            "{\"error\":\"Unable to scan Wi-Fi networks right now.\"}");
    }

    char response[MANAGEMENT_WIFI_SCAN_RESPONSE_SIZE];
    size_t used = 0;
    bool response_valid = management_json_append(
        response, sizeof(response), &used, "{\"networks\":[");
    for (size_t index = 0; response_valid && index < results.count; index++)
    {
        const WifiManagementScanResult *entry = &results.entries[index];
        response_valid = management_json_append(
            response, sizeof(response), &used, "%s{\"ssid\":",
            index == 0U ? "" : ",");
        response_valid = response_valid && management_json_append_string(
                                             response, sizeof(response), &used,
                                             entry->ssid);
        response_valid = response_valid && management_json_append(
                                             response, sizeof(response), &used,
                                             ",\"rssi_dbm\":%d,\"security\":",
                                             entry->rssi_dbm);
        response_valid = response_valid && management_json_append_string(
                                             response, sizeof(response), &used,
                                             management_wifi_security_name(
                                                 entry->authmode));
        response_valid = response_valid && management_json_append(
                                             response, sizeof(response), &used,
                                             "}");
    }
    response_valid = response_valid && management_json_append(
                                         response, sizeof(response), &used,
                                         "],\"maximum\":%u}",
                                         (unsigned int)WIFI_MANAGEMENT_SCAN_RESULT_LIMIT);
    mbedtls_platform_zeroize(&results, sizeof(results));
    if (!response_valid)
    {
        mbedtls_platform_zeroize(response, sizeof(response));
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to prepare the Wi-Fi scan response.\"}");
    }

    const esp_err_t send_result = management_send_json(request, "200 OK", response);
    mbedtls_platform_zeroize(response, sizeof(response));
    return send_result;
}

esp_err_t management_wifi_configure_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(
            request, "403 Forbidden",
            "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1] = {0};
    char ssid[WIFI_MANAGEMENT_SSID_MAX_LENGTH + 1U] = {0};
    char password[WIFI_MANAGEMENT_PASSWORD_MAX_LENGTH + 1U] = {0};
    char acknowledgement[6] = {0};
    const esp_err_t form_result =
        management_read_form_body(request, body, sizeof(body));
    const bool fields_present =
        form_result == ESP_OK &&
        management_form_value(body, "ssid", ssid, sizeof(ssid)) &&
        management_form_value(body, "password", password, sizeof(password)) &&
        management_form_value(body, "acknowledge", acknowledgement,
                              sizeof(acknowledgement));
    mbedtls_platform_zeroize(body, sizeof(body));
    if (!fields_present || strcmp(acknowledgement, "true") != 0)
    {
        mbedtls_platform_zeroize(ssid, sizeof(ssid));
        mbedtls_platform_zeroize(password, sizeof(password));
        mbedtls_platform_zeroize(acknowledgement, sizeof(acknowledgement));
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"Wi-Fi changes require explicit confirmation.\"}");
    }

    const esp_err_t result = wifi_management_stage_credentials(ssid, password);
    mbedtls_platform_zeroize(ssid, sizeof(ssid));
    mbedtls_platform_zeroize(password, sizeof(password));
    mbedtls_platform_zeroize(acknowledgement, sizeof(acknowledgement));
    if (result == ESP_ERR_INVALID_ARG)
    {
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"Use a 1-32 character network name and an 8-63 character password, or leave the password blank for an open network. Enter a password when keeping the current secured network.\"}");
    }
    if (result == ESP_ERR_INVALID_STATE)
    {
        return management_send_json(
            request, "409 Conflict",
            "{\"error\":\"Wi-Fi is not currently connected; reconnect before changing networks.\"}");
    }
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to stage Wi-Fi credentials: %s",
                 esp_err_to_name(result));
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to stage Wi-Fi credentials. Try again.\"}");
    }
    return management_send_json(
        request, "202 Accepted",
        "{\"message\":\"Wi-Fi credentials staged. The device will restart and test the new network; the previous network remains the fallback if validation fails.\"}");
}
