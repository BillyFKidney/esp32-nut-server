#include "management.h"
#include "management-auth-routes.h"
#include "management-certificates.h"
#include "management-credentials.h"
#include "management-http.h"
#include "management-log.h"
#include "management-pages.h"
#include "management-session.h"
#include "management-status.h"
#include "management-authorization.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "api_tokens.h"
#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"
#include "ota.h"
#include "time_config.h"
#include "wifi-provisioning.h"

#define TAG "nut-management"

#define MANAGEMENT_NAMESPACE "management"
#define MANAGEMENT_DEVICE_NAME_KEY "device-name"

#define MANAGEMENT_DEFAULT_DEVICE_NAME "ESP32-NUT"
#define MANAGEMENT_HTTPS_PORT 443
#define MANAGEMENT_HTTPS_REQUEST_HEADER_LIMIT 4096U
#define MANAGEMENT_STATUS_RESPONSE_SIZE 7000
#define MANAGEMENT_WIFI_SCAN_RESPONSE_SIZE 4200
#define MANAGEMENT_HTTPS_ROUTE_CAPACITY 17

_Static_assert(sizeof(MANAGEMENT_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "Management NVS namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_DEVICE_NAME_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "Device-name NVS key exceeds the ESP-IDF limit");

static httpd_handle_t management_https_server;

static esp_err_t management_open_nvs(nvs_open_mode_t mode, nvs_handle_t *handle)
{
    return nvs_open(MANAGEMENT_NAMESPACE, mode, handle);
}


static esp_err_t management_root_handler(httpd_req_t *request)
{
    if (!management_admin_password_is_configured())
    {
        char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
        char setup_header[192];
        management_session_start_setup(request, csrf, sizeof(csrf), setup_header,
                                       sizeof(setup_header));
        const esp_err_t send_result =
            management_pages_send_setup(request, csrf);
        mbedtls_platform_zeroize(csrf, sizeof(csrf));
        return send_result;
    }
    if (!management_session_is_authorized(request, true))
    {
        management_session_expire_cookie(request);
        const int retry_after =
            management_session_login_retry_after_seconds(esp_timer_get_time());
        if (retry_after > 0)
        {
            return management_pages_send_login_throttled(request, retry_after);
        }
        return management_pages_send_login(request);
    }

    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    management_session_copy_csrf(csrf, sizeof(csrf));
    const esp_err_t send_result = management_pages_send_admin(request, csrf);
    mbedtls_platform_zeroize(csrf, sizeof(csrf));
    return send_result;
}

static esp_err_t management_logout_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden", "{\"error\":\"Invalid session or CSRF token.\"}");
    }
    management_session_clear();
    management_session_expire_cookie(request);
    return management_send_redirect(request, "/");
}

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

static esp_err_t management_wifi_scan_handler(httpd_req_t *request)
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

static esp_err_t management_wifi_configure_handler(httpd_req_t *request)
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

static esp_err_t management_status_handler(httpd_req_t *request)
{
    if (!management_require_session_without_activity(request))
    {
        return ESP_OK;
    }

    esp_netif_ip_info_t ip_info = {0};
    esp_netif_t *station = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (station != NULL)
    {
        esp_netif_get_ip_info(station, &ip_info);
    }
    wifi_ap_record_t access_point = {0};
    const esp_err_t access_point_result = esp_wifi_sta_get_ap_info(&access_point);
    const esp_app_desc_t *app_description = esp_app_get_description();
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    const esp_partition_t *next_partition = esp_ota_get_next_update_partition(NULL);
    TimeConfigStatus time_status;
    time_config_get_status(&time_status);
    const uint32_t session_remaining_seconds = management_session_remaining_seconds();
    const bool session_warning = session_remaining_seconds > 0 &&
                                 session_remaining_seconds <= MANAGEMENT_SESSION_WARNING_SECONDS;
    ManagementStatusNutSnapshot nut_snapshot;
    management_status_collect_nut_snapshot(&nut_snapshot);
    management_status_initialize_hardware_diagnostics();
    ManagementStatusHardwareSnapshot hardware_snapshot;
    management_status_collect_hardware_snapshot(&hardware_snapshot);
    char last_update_result[32] = {0};
    if (ota_get_last_result(last_update_result, sizeof(last_update_result)) != ESP_OK)
    {
        snprintf(last_update_result, sizeof(last_update_result), "unavailable");
    }
    char address[16] = "unassigned";
    char ssid[33] = "";
    if (ip_info.ip.addr != 0)
    {
        snprintf(address, sizeof(address), IPSTR, IP2STR(&ip_info.ip));
    }
    if (access_point_result == ESP_OK)
    {
        memcpy(ssid, access_point.ssid, sizeof(access_point.ssid));
        ssid[sizeof(ssid) - 1U] = '\0';
    }

    const char *nut_health = nut_snapshot.available ? "ok" :
                             (nut_snapshot.stale ? "stale" : "unavailable");
    char response[MANAGEMENT_STATUS_RESPONSE_SIZE];
    size_t used = 0;
    bool response_valid = true;
#define MANAGEMENT_JSON_APPEND(...) \
    response_valid = response_valid && \
                     management_json_append(response, sizeof(response), &used, __VA_ARGS__)
#define MANAGEMENT_JSON_STRING(value) \
    response_valid = response_valid && \
                     management_json_append_string(response, sizeof(response), &used, value)

    MANAGEMENT_JSON_APPEND("{\"device_name\":");
    MANAGEMENT_JSON_STRING(MANAGEMENT_DEFAULT_DEVICE_NAME);
    MANAGEMENT_JSON_APPEND(",\"firmware\":");
    MANAGEMENT_JSON_STRING(app_description != NULL ? app_description->version : "unknown");
    MANAGEMENT_JSON_APPEND(",\"uptime_seconds\":%lld,\"wifi\":{\"ip\":",
                           (long long)(esp_timer_get_time() / 1000000LL));
    MANAGEMENT_JSON_STRING(address);
    MANAGEMENT_JSON_APPEND(",\"ssid\":");
    MANAGEMENT_JSON_STRING(ssid);
    MANAGEMENT_JSON_APPEND(",\"rssi_dbm\":%d,\"connected\":%s},"
                           "\"management\":{\"transport\":\"https\","
                           "\"certificate\":\"self-signed\",\"role\":\"ADMIN\"},"
                           "\"time\":{\"available\":%s,\"utc\":",
                           access_point_result == ESP_OK ? access_point.rssi : 0,
                           access_point_result == ESP_OK ? "true" : "false",
                           time_status.available ? "true" : "false");
    MANAGEMENT_JSON_STRING(time_status.utc);
    MANAGEMENT_JSON_APPEND(",\"local\":");
    MANAGEMENT_JSON_STRING(time_status.local);
    MANAGEMENT_JSON_APPEND(",\"timezone\":");
    MANAGEMENT_JSON_STRING(time_status.timezone);
    MANAGEMENT_JSON_APPEND(",\"source\":");
    MANAGEMENT_JSON_STRING(time_status.source);
    MANAGEMENT_JSON_APPEND(",\"ntp_enabled\":%s,\"ntp_server\":",
                           time_status.ntp_enabled ? "true" : "false");
    MANAGEMENT_JSON_STRING(time_status.ntp_server);
    MANAGEMENT_JSON_APPEND(",\"ntp_synchronized\":%s,\"synchronization_pending\":%s},"
                           "\"ota\":{\"running_slot\":",
                           time_status.ntp_synchronized ? "true" : "false",
                           time_status.synchronization_pending ? "true" : "false");
    MANAGEMENT_JSON_STRING(running_partition != NULL ? running_partition->label : "unknown");
    MANAGEMENT_JSON_APPEND(",\"next_slot\":");
    MANAGEMENT_JSON_STRING(next_partition != NULL ? next_partition->label : "unavailable");
    MANAGEMENT_JSON_APPEND("},\"update\":{\"last_result\":");
    MANAGEMENT_JSON_STRING(last_update_result);
    MANAGEMENT_JSON_APPEND("},\"session\":{\"idle_timeout_seconds\":%u,"
                           "\"remaining_seconds\":%u,\"warning\":%s},"
                           "\"hardware\":{\"chip\":{\"model\":",
                           MANAGEMENT_SESSION_IDLE_SECONDS,
                           session_remaining_seconds,
                           session_warning ? "true" : "false");
    MANAGEMENT_JSON_STRING(management_status_chip_model_name(hardware_snapshot.chip.model));
    MANAGEMENT_JSON_APPEND(",\"revision\":%u,\"cores\":%u,\"features\":{",
                           (unsigned int)hardware_snapshot.chip.revision,
                           (unsigned int)hardware_snapshot.chip.cores);
    MANAGEMENT_JSON_APPEND("\"embedded_flash\":%s,\"wifi_bgn\":%s,",
                           (hardware_snapshot.chip.features & CHIP_FEATURE_EMB_FLASH) != 0 ? "true" : "false",
                           (hardware_snapshot.chip.features & CHIP_FEATURE_WIFI_BGN) != 0 ? "true" : "false");
    MANAGEMENT_JSON_APPEND("\"bluetooth_classic\":%s,\"bluetooth_le\":%s,",
                           (hardware_snapshot.chip.features & CHIP_FEATURE_BT) != 0 ? "true" : "false",
                           (hardware_snapshot.chip.features & CHIP_FEATURE_BLE) != 0 ? "true" : "false");
    MANAGEMENT_JSON_APPEND("\"ieee802154\":%s,\"embedded_psram\":%s}},",
                           (hardware_snapshot.chip.features & CHIP_FEATURE_IEEE802154) != 0 ? "true" : "false",
                           (hardware_snapshot.chip.features & CHIP_FEATURE_EMB_PSRAM) != 0 ? "true" : "false");
    MANAGEMENT_JSON_APPEND("\"board\":{\"profile\":");
    MANAGEMENT_JSON_STRING(hardware_snapshot.board_profile);
    MANAGEMENT_JSON_APPEND(",\"module\":");
    MANAGEMENT_JSON_STRING(hardware_snapshot.module_profile);
    MANAGEMENT_JSON_APPEND("},\"flash\":{\"size_bytes\":%lu,\"mode\":",
                           (unsigned long)hardware_snapshot.flash_size_bytes);
    MANAGEMENT_JSON_STRING(hardware_snapshot.flash_mode);
    MANAGEMENT_JSON_APPEND(",\"frequency\":");
    MANAGEMENT_JSON_STRING(hardware_snapshot.flash_frequency);
    MANAGEMENT_JSON_APPEND("},\"psram\":{\"available\":%s,\"size_bytes\":%lu,\"mode\":",
                           hardware_snapshot.psram_size_bytes > 0 ? "true" : "false",
                           (unsigned long)hardware_snapshot.psram_size_bytes);
    MANAGEMENT_JSON_STRING(hardware_snapshot.psram_mode);
    MANAGEMENT_JSON_APPEND(",\"frequency_mhz\":%d},\"memory\":{"
                           "\"free_internal_bytes\":%lu,\"free_psram_bytes\":%lu,"
                           "\"minimum_free_bytes\":%lu},\"chip_temperature\":{"
                           "\"available\":%s,\"celsius\":",
                           hardware_snapshot.psram_frequency_mhz,
                           (unsigned long)hardware_snapshot.free_internal_heap_bytes,
                           (unsigned long)hardware_snapshot.free_psram_bytes,
                           (unsigned long)hardware_snapshot.minimum_free_heap_bytes,
                           hardware_snapshot.chip_temperature_available ? "true" : "false");
    if (hardware_snapshot.chip_temperature_available)
    {
        MANAGEMENT_JSON_APPEND("%.1f", (double)hardware_snapshot.chip_temperature_celsius);
    }
    else
    {
        MANAGEMENT_JSON_APPEND("null");
    }
    MANAGEMENT_JSON_APPEND("}}");
    if (!management_log_append_snapshot(response, sizeof(response), &used))
    {
        response_valid = false;
    }
    MANAGEMENT_JSON_APPEND(",\"nut\":{\"port\":3493,\"mode\":\"read-only\","
                           "\"available\":%s,\"data_stale\":%s,\"health\":",
                           nut_snapshot.available ? "true" : "false",
                           nut_snapshot.stale ? "true" : "false");
    MANAGEMENT_JSON_STRING(nut_health);
    MANAGEMENT_JSON_APPEND(",\"ups_name\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.ups_name);
    MANAGEMENT_JSON_APPEND("},\"ups\":{\"manufacturer\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.manufacturer);
    MANAGEMENT_JSON_APPEND(",\"model\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.model);
    MANAGEMENT_JSON_APPEND(",\"serial\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.serial);
    MANAGEMENT_JSON_APPEND(",\"status\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.status);
    MANAGEMENT_JSON_APPEND(",\"battery_type\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.battery_type);
    MANAGEMENT_JSON_APPEND(",\"battery_mfr_date\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.battery_mfr_date);
    MANAGEMENT_JSON_APPEND(",\"temperature\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.ups_temperature);
    MANAGEMENT_JSON_APPEND(",\"battery_charge\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.battery_charge);
    MANAGEMENT_JSON_APPEND(",\"battery_runtime\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.battery_runtime);
    MANAGEMENT_JSON_APPEND(",\"battery_voltage\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.battery_voltage);
    MANAGEMENT_JSON_APPEND(",\"load\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.load);
    MANAGEMENT_JSON_APPEND(",\"input_voltage\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.input_voltage);
    MANAGEMENT_JSON_APPEND(",\"output_voltage\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.output_voltage);
    MANAGEMENT_JSON_APPEND(",\"power\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.ups_power);
    MANAGEMENT_JSON_APPEND(",\"realpower\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.ups_realpower);
    MANAGEMENT_JSON_APPEND(",\"firmware\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.ups_firmware);
    MANAGEMENT_JSON_APPEND("}}");

#undef MANAGEMENT_JSON_STRING
#undef MANAGEMENT_JSON_APPEND

    if (!response_valid)
    {
        mbedtls_platform_zeroize(response, sizeof(response));
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to prepare device status.\"}");
    }
    const esp_err_t send_result = management_send_json(request, "200 OK", response);
    mbedtls_platform_zeroize(response, sizeof(response));
    return send_result;
}

static esp_err_t management_session_activity_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(
            request, "403 Forbidden",
            "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char response[96];
    const uint32_t remaining_seconds = management_session_remaining_seconds();
    snprintf(response, sizeof(response),
             "{\"remaining_seconds\":%u,\"warning\":%s}",
             (unsigned int)remaining_seconds,
             remaining_seconds > 0 &&
                     remaining_seconds <= MANAGEMENT_SESSION_WARNING_SECONDS
                 ? "true"
                 : "false");
    return management_send_json(request, "200 OK", response);
}

static esp_err_t management_token_list_handler(httpd_req_t *request)
{
    if (!management_require_session(request, true))
    {
        return ESP_OK;
    }

    ApiTokenList list;
    const esp_err_t result = api_tokens_list(&list);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to list API-token metadata: %s",
                 esp_err_to_name(result));
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to load API-token metadata.\"}");
    }

    char response[1400];
    int written = snprintf(response, sizeof(response), "{\"tokens\":[");
    size_t used = written > 0 ? (size_t)written : sizeof(response);
    for (size_t index = 0; index < list.count && used < sizeof(response); index++)
    {
        const ApiTokenMetadata *token = &list.tokens[index];
        written = snprintf(
            response + used, sizeof(response) - used,
            "%s{\"id\":\"%s\",\"name\":\"%s\",\"issued_at\":\"%s\","
            "\"final_four\":\"%s\",\"scopes\":[\"ota.install\"]}",
            index == 0U ? "" : ",", token->id, token->name,
            token->issued_at, token->final_four);
        if (written < 0 || (size_t)written >= sizeof(response) - used)
        {
            used = sizeof(response);
            break;
        }
        used += (size_t)written;
    }
    if (used < sizeof(response))
    {
        written = snprintf(response + used, sizeof(response) - used,
                           "],\"maximum\":%u}",
                           (unsigned int)API_TOKEN_MAX_COUNT);
    }
    mbedtls_platform_zeroize(&list, sizeof(list));
    if (used >= sizeof(response) || written < 0 ||
        (size_t)written >= sizeof(response) - used)
    {
        mbedtls_platform_zeroize(response, sizeof(response));
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to prepare API-token metadata.\"}");
    }

    const esp_err_t send_result =
        management_send_json(request, "200 OK", response);
    mbedtls_platform_zeroize(response, sizeof(response));
    return send_result;
}

static esp_err_t management_token_create_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(
            request, "403 Forbidden",
            "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char name[API_TOKEN_NAME_MAX_LENGTH + 1U] = {0};
    const esp_err_t form_result =
        management_read_form_body(request, body, sizeof(body));
    const bool name_present =
        form_result == ESP_OK &&
        management_form_value(body, "name", name, sizeof(name));
    mbedtls_platform_zeroize(body, sizeof(body));
    if (!name_present || !api_token_name_is_valid(name))
    {
        mbedtls_platform_zeroize(name, sizeof(name));
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"Use a unique 1-32 character token name containing letters, numbers, spaces, periods, underscores, or hyphens.\"}");
    }

    TimeConfigStatus time_status;
    time_config_get_status(&time_status);
    if (!time_status.available)
    {
        mbedtls_platform_zeroize(name, sizeof(name));
        return management_send_json(
            request, "409 Conflict",
            "{\"error\":\"Set or synchronize device time before creating an API token.\"}");
    }

    ApiTokenMetadata metadata;
    char token[API_TOKEN_VALUE_LENGTH + 1U] = {0};
    const esp_err_t result =
        api_tokens_create(name, time(NULL), API_TOKEN_SCOPE_OTA_INSTALL,
                          &metadata, token);
    mbedtls_platform_zeroize(name, sizeof(name));
    if (result == ESP_ERR_INVALID_STATE)
    {
        mbedtls_platform_zeroize(token, sizeof(token));
        return management_send_json(
            request, "409 Conflict",
            "{\"error\":\"An active API token already uses that name.\"}");
    }
    if (result == ESP_ERR_NO_MEM)
    {
        mbedtls_platform_zeroize(token, sizeof(token));
        return management_send_json(
            request, "409 Conflict",
            "{\"error\":\"The maximum of four active API tokens has been reached.\"}");
    }
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to create API token: %s", esp_err_to_name(result));
        mbedtls_platform_zeroize(token, sizeof(token));
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to create the API token.\"}");
    }

    char response[420];
    const int response_length = snprintf(
        response, sizeof(response),
        "{\"token\":\"%s\",\"id\":\"%s\",\"name\":\"%s\","
        "\"issued_at\":\"%s\",\"final_four\":\"%s\","
        "\"scopes\":[\"ota.install\"]}",
        token, metadata.id, metadata.name, metadata.issued_at,
        metadata.final_four);
    esp_err_t send_result;
    if (response_length < 0 || response_length >= (int)sizeof(response))
    {
        send_result = management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"The API token was created but its one-time response could not be prepared. Delete the undisclosed token and create another.\"}");
    }
    else
    {
        send_result = management_send_json(request, "201 Created", response);
    }
    mbedtls_platform_zeroize(response, sizeof(response));
    mbedtls_platform_zeroize(token, sizeof(token));
    mbedtls_platform_zeroize(&metadata, sizeof(metadata));
    return send_result;
}

static esp_err_t management_token_delete_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(
            request, "403 Forbidden",
            "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char id[API_TOKEN_ID_HEX_LENGTH + 1U] = {0};
    char acknowledgement[6] = {0};
    const esp_err_t form_result =
        management_read_form_body(request, body, sizeof(body));
    const bool fields_present =
        form_result == ESP_OK &&
        management_form_value(body, "id", id, sizeof(id)) &&
        management_form_value(body, "acknowledge", acknowledgement,
                              sizeof(acknowledgement));
    mbedtls_platform_zeroize(body, sizeof(body));
    if (!fields_present || strcmp(acknowledgement, "true") != 0)
    {
        mbedtls_platform_zeroize(id, sizeof(id));
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"Token deletion requires the acknowledgement checkbox and explicit confirmation.\"}");
    }

    const esp_err_t result = api_tokens_delete(id);
    mbedtls_platform_zeroize(id, sizeof(id));
    if (result == ESP_ERR_INVALID_ARG)
    {
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"A valid API-token identifier is required.\"}");
    }
    if (result == ESP_ERR_NOT_FOUND)
    {
        return management_send_json(
            request, "404 Not Found",
            "{\"error\":\"The API token is no longer active.\"}");
    }
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to delete API token: %s", esp_err_to_name(result));
        return management_send_json(
            request, "500 Internal Server Error",
            "{\"error\":\"Unable to delete the API token.\"}");
    }
    return management_send_json(
        request, "200 OK",
        "{\"message\":\"API token deleted and revoked.\"}");
}

static esp_err_t management_time_config_handler(httpd_req_t *request)
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

static esp_err_t management_ota_install_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden", "{\"error\":\"Invalid session or CSRF token.\"}");
    }
    return ota_install_from_request(request);
}

static esp_err_t management_ota_check_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden", "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    static const char expected_content_type[] = "application/octet-stream";
    char content_type[sizeof(expected_content_type)] = {0};
    if (httpd_req_get_hdr_value_len(request, "Content-Type") !=
            sizeof(expected_content_type) - 1U ||
        httpd_req_get_hdr_value_str(request, "Content-Type", content_type,
                                    sizeof(content_type)) != ESP_OK ||
        strcmp(content_type, expected_content_type) != 0)
    {
        return management_send_json(
            request, "415 Unsupported Media Type",
            "{\"error\":\"Firmware check requires an application/octet-stream image body.\"}");
    }
    return ota_check_from_request(request);
}

static esp_err_t management_agent_ota_install_handler(httpd_req_t *request)
{
    if (!management_bearer_is_authorized(request,
                                         API_TOKEN_SCOPE_OTA_INSTALL))
    {
        return management_send_bearer_unauthorized(request);
    }

    static const char expected_content_type[] = "application/octet-stream";
    char content_type[sizeof(expected_content_type)] = {0};
    if (httpd_req_get_hdr_value_len(request, "Content-Type") !=
            sizeof(expected_content_type) - 1U ||
        httpd_req_get_hdr_value_str(request, "Content-Type", content_type,
                                    sizeof(content_type)) != ESP_OK ||
        strcmp(content_type, expected_content_type) != 0)
    {
        return management_send_json(
            request, "415 Unsupported Media Type",
            "{\"error\":\"Agent OTA requires an application/octet-stream firmware body.\"}");
    }
    return ota_install_from_request(request);
}

esp_err_t management_factory_reset(void)
{
    nvs_handle_t handle = 0;
    esp_err_t result = management_open_nvs(NVS_READWRITE, &handle);
    if (result != ESP_OK)
    {
        return result;
    }
    result = nvs_erase_all(handle);
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    management_session_clear();
    management_session_record_login_success();
    return result;
}

esp_err_t management_server_start(void)
{
    if (management_https_server != NULL)
    {
        return ESP_OK;
    }

    management_status_initialize_hardware_diagnostics();

    ESP_RETURN_ON_ERROR(management_certificates_load_or_create(), TAG,
                        "Unable to prepare HTTPS certificate");

    const ManagementCertificateMaterial *certificate_material =
        management_certificates_get_material();

    httpd_ssl_config_t configuration = HTTPD_SSL_CONFIG_DEFAULT();
    configuration.httpd.server_port = MANAGEMENT_HTTPS_PORT;
    configuration.httpd.stack_size = 12288;
    /*
     * Chrome plus a trusted reverse proxy can exceed ESP-IDF's 1024-byte
     * default before a setup or authentication handler receives the request.
     * This remains a bounded, management-server-only limit; the HTTP captive
     * portal retains its smaller default.
     */
    configuration.httpd.max_req_hdr_len = MANAGEMENT_HTTPS_REQUEST_HEADER_LIMIT;
    configuration.httpd.max_open_sockets = 4;
    configuration.httpd.max_uri_handlers = MANAGEMENT_HTTPS_ROUTE_CAPACITY;
    configuration.httpd.lru_purge_enable = true;
    configuration.servercert = certificate_material->certificate;
    configuration.servercert_len = certificate_material->certificate_length;
    configuration.prvtkey_pem = certificate_material->private_key;
    configuration.prvtkey_len = certificate_material->private_key_length;

    esp_err_t result = httpd_ssl_start(&management_https_server, &configuration);
    if (result != ESP_OK)
    {
        management_https_server = NULL;
        ESP_LOGE(TAG, "Unable to start the HTTPS management server: %s", esp_err_to_name(result));
        return result;
    }

    const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = management_root_handler};
    const httpd_uri_t setup = {.uri = "/setup", .method = HTTP_POST, .handler = management_auth_setup_handler};
    const httpd_uri_t login_page = {.uri = "/login", .method = HTTP_GET, .handler = management_auth_login_page_handler};
    const httpd_uri_t login = {.uri = "/login", .method = HTTP_POST, .handler = management_auth_login_handler};
    const httpd_uri_t password = {.uri = "/api/v1/admin/password", .method = HTTP_POST, .handler = management_auth_password_change_handler};
    const httpd_uri_t logout = {.uri = "/logout", .method = HTTP_POST, .handler = management_logout_handler};
    const httpd_uri_t status = {.uri = "/api/v1/status", .method = HTTP_GET, .handler = management_status_handler};
    const httpd_uri_t session_activity = {.uri = "/api/v1/admin/session/activity", .method = HTTP_POST, .handler = management_session_activity_handler};
    const httpd_uri_t time_configuration = {.uri = "/api/v1/admin/time", .method = HTTP_POST, .handler = management_time_config_handler};
    const httpd_uri_t ota_check = {.uri = "/api/v1/ota/check", .method = HTTP_POST, .handler = management_ota_check_handler};
    const httpd_uri_t ota = {.uri = "/api/v1/ota/install", .method = HTTP_POST, .handler = management_ota_install_handler};
    const httpd_uri_t token_list = {.uri = "/api/v1/admin/tokens", .method = HTTP_GET, .handler = management_token_list_handler};
    const httpd_uri_t token_create = {.uri = "/api/v1/admin/tokens", .method = HTTP_POST, .handler = management_token_create_handler};
    const httpd_uri_t token_delete = {.uri = "/api/v1/admin/tokens", .method = HTTP_DELETE, .handler = management_token_delete_handler};
    const httpd_uri_t wifi_scan = {.uri = "/api/v1/admin/wifi/scan", .method = HTTP_GET, .handler = management_wifi_scan_handler};
    const httpd_uri_t wifi_configuration = {.uri = "/api/v1/admin/wifi", .method = HTTP_POST, .handler = management_wifi_configure_handler};
    const httpd_uri_t agent_ota = {.uri = "/api/v1/agent/ota/install", .method = HTTP_POST, .handler = management_agent_ota_install_handler};
    const httpd_uri_t *routes[] = {
        &root, &setup, &login_page, &login, &password, &logout, &status,
        &session_activity, &time_configuration, &ota_check, &ota, &token_list, &token_create, &token_delete,
        &wifi_scan, &wifi_configuration, &agent_ota};
    _Static_assert(sizeof(routes) / sizeof(routes[0]) <=
                       MANAGEMENT_HTTPS_ROUTE_CAPACITY,
                   "HTTPS route count exceeds configured handler capacity");
    for (size_t index = 0; index < sizeof(routes) / sizeof(routes[0]); index++)
    {
        result = httpd_register_uri_handler(management_https_server, routes[index]);
        if (result != ESP_OK)
        {
            ESP_LOGE(TAG, "Unable to register HTTPS management route: %s", esp_err_to_name(result));
            httpd_ssl_stop(management_https_server);
            management_https_server = NULL;
            return result;
        }
    }

    ESP_LOGI(TAG, "LAN-only HTTPS administration is listening on TCP port %d", MANAGEMENT_HTTPS_PORT);
    return ESP_OK;
}
