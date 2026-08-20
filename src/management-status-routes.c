/** @file management-status-routes.c @brief Serve authenticated management status JSON. @see management-status-routes.h, management-authorization.h, management-status.h, time_config.h */
#include "management-status-routes.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "management-authorization.h"
#include "management-http.h"
#include "management-log.h"
#include "management-session.h"
#include "management-status.h"

#include "esp_app_desc.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "mbedtls/platform_util.h"
#include "ota.h"
#include "time_config.h"

#define MANAGEMENT_DEFAULT_DEVICE_NAME "ESP32-NUT"
#define MANAGEMENT_STATUS_RESPONSE_SIZE 7000U

static esp_err_t management_status_send(httpd_req_t *request)
{
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
                           "\"available\":%s,\"data_stale\":%s,\"disconnect_simulated\":%s,\"health\":",
                           nut_snapshot.available ? "true" : "false",
                           nut_snapshot.stale ? "true" : "false",
                           nut_snapshot.disconnect_simulated ? "true" : "false");
    MANAGEMENT_JSON_STRING(nut_health);
    MANAGEMENT_JSON_APPEND(",\"ups\":");
    MANAGEMENT_JSON_STRING(nut_snapshot.ups);
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

esp_err_t management_status_handler(httpd_req_t *request)
{
    if (!management_require_session_without_activity(request))
    {
        return ESP_OK;
    }
    return management_status_send(request);
}

esp_err_t management_agent_status_handler(httpd_req_t *request)
{
    if (!management_diagnostic_bearer_is_authorized(request))
    {
        return management_send_diagnostic_bearer_unauthorized(request);
    }
    return management_status_send(request);
}
