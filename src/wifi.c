#include "wifi-provisioning.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dns-server.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/prot/dhcp.h"
#include "lwip/tcpip.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "management.h"
#include "wifi-portal.h"

#define WIFI_CONFIG_NAMESPACE "wifi-config"
#define WIFI_SSID_KEY "ssid"
#define WIFI_PASSWORD_KEY "password"
#define WIFI_PENDING_SSID_KEY "pending-ssid"
#define WIFI_PENDING_PASSWORD_KEY "pending-pass"
#define WIFI_AP_INTERFACE_KEY "WIFI_AP_DEF"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONNECTIONS 4
#define WIFI_MAXIMUM_RETRIES 5
#define WIFI_SAVED_CONNECT_TIMEOUT_MS 30000
#define WIFI_PENDING_CONNECT_TIMEOUT_MS 20000
#define WIFI_PORTAL_START_TIMEOUT_MS 5000
#define WIFI_PORTAL_TASK_STACK_SIZE 4096
#define WIFI_RESTART_TASK_STACK_SIZE 2048
#define WIFI_SCAN_RESULT_LIMIT 20
#define WIFI_REQUEST_BODY_LIMIT 256
#define WIFI_CONNECTION_DIAGNOSTIC_LENGTH 192
#define WIFI_BOOT_BUTTON GPIO_NUM_0
#define WIFI_BOOT_RESET_HOLD_MS 3000
#define WIFI_BOOT_FACTORY_RESET_HOLD_MS 15000

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT BIT1
#define WIFI_PORTAL_STARTED_BIT BIT2

static const char *TAG = "nut-wifi";

typedef struct
{
    char ssid[33];
    char password[64];
} WifiCredentials;

typedef struct
{
    struct netif *network_interface;
    bool available;
    bool offer_received;
    uint8_t state;
    uint8_t tries;
} WifiDhcpSnapshot;

static EventGroupHandle_t wifi_event_group;
static esp_netif_t *station_network_interface;
static esp_netif_t *access_point_network_interface;
static httpd_handle_t portal_http_server;
static DnsServerHandle portal_dns_server;
static bool connection_requested;
static bool portal_active;
static bool portal_start_scheduled;
static unsigned int connection_retry_count;
static bool station_associated;
static char connection_diagnostic[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
static portMUX_TYPE wifi_state_lock = portMUX_INITIALIZER_UNLOCKED;

static void wifi_schedule_portal(void);

static void wifi_set_connection_diagnostic(const char *message)
{
    taskENTER_CRITICAL(&wifi_state_lock);
    snprintf(connection_diagnostic, sizeof(connection_diagnostic), "%s", message);
    taskEXIT_CRITICAL(&wifi_state_lock);
}

static void wifi_get_connection_diagnostic(char *destination, size_t destination_size)
{
    taskENTER_CRITICAL(&wifi_state_lock);
    snprintf(destination, destination_size, "%s", connection_diagnostic);
    taskEXIT_CRITICAL(&wifi_state_lock);
}

static const char *wifi_dhcp_state_name(uint8_t state)
{
    switch (state)
    {
        case DHCP_STATE_REQUESTING:
            return "REQUESTING";
        case DHCP_STATE_INIT:
            return "INIT";
        case DHCP_STATE_REBOOTING:
            return "REBOOTING";
        case DHCP_STATE_REBINDING:
            return "REBINDING";
        case DHCP_STATE_RENEWING:
            return "RENEWING";
        case DHCP_STATE_SELECTING:
            return "SELECTING";
        case DHCP_STATE_INFORMING:
            return "INFORMING";
        case DHCP_STATE_CHECKING:
            return "CHECKING";
        case DHCP_STATE_BOUND:
            return "BOUND";
        case DHCP_STATE_BACKING_OFF:
            return "BACKING_OFF";
        case DHCP_STATE_OFF:
        default:
            return "OFF";
    }
}

static void wifi_capture_dhcp_snapshot_callback(void *argument)
{
    WifiDhcpSnapshot *snapshot = argument;
    const struct dhcp *dhcp = netif_dhcp_data(snapshot->network_interface);
    if (dhcp == NULL)
    {
        return;
    }

    snapshot->available = true;
    snapshot->state = dhcp->state;
    snapshot->tries = dhcp->tries;
    snapshot->offer_received = !ip4_addr_isany_val(dhcp->offered_ip_addr);
}

static void wifi_capture_dhcp_diagnostic(void)
{
    WifiDhcpSnapshot snapshot = {
        .network_interface = (struct netif *)esp_netif_get_netif_impl(station_network_interface),
    };
    if (snapshot.network_interface == NULL ||
        tcpip_callback_wait(wifi_capture_dhcp_snapshot_callback, &snapshot) != ERR_OK)
    {
        wifi_set_connection_diagnostic("Wi-Fi associated, but the DHCP client state could not be inspected.");
        return;
    }

    if (!snapshot.available)
    {
        wifi_set_connection_diagnostic("Wi-Fi associated, but the DHCP client was not running.");
        return;
    }

    if ((snapshot.state == DHCP_STATE_SELECTING || snapshot.state == DHCP_STATE_BACKING_OFF) &&
        !snapshot.offer_received)
    {
        char message[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
        snprintf(message, sizeof(message),
                 "Wi-Fi associated, but no DHCP offer was received (%s after %u attempts).",
                 wifi_dhcp_state_name(snapshot.state), snapshot.tries);
        wifi_set_connection_diagnostic(message);
        return;
    }

    if (snapshot.state == DHCP_STATE_REQUESTING && snapshot.offer_received)
    {
        wifi_set_connection_diagnostic("A DHCP offer was received, but its lease acknowledgement did not arrive.");
        return;
    }

    if (snapshot.state == DHCP_STATE_CHECKING && snapshot.offer_received)
    {
        wifi_set_connection_diagnostic("A DHCP acknowledgement was received; the offered address is being checked for a conflict.");
        return;
    }

    char message[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
    snprintf(message, sizeof(message),
             "Wi-Fi associated, but DHCP did not complete (state %s, %u attempts, offer %s).",
             wifi_dhcp_state_name(snapshot.state), snapshot.tries,
             snapshot.offer_received ? "received" : "not received");
    wifi_set_connection_diagnostic(message);
}

static void nvs_initialize(void)
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(result);
}

static bool wifi_credentials_load(WifiCredentials *credentials)
{
    nvs_handle_t handle;
    if (nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    size_t ssid_length = sizeof(credentials->ssid);
    size_t password_length = sizeof(credentials->password);
    const esp_err_t ssid_result = nvs_get_str(handle, WIFI_SSID_KEY, credentials->ssid, &ssid_length);
    const esp_err_t password_result = nvs_get_str(handle, WIFI_PASSWORD_KEY, credentials->password, &password_length);
    nvs_close(handle);

    return ssid_result == ESP_OK && password_result == ESP_OK && credentials->ssid[0] != '\0';
}

static bool wifi_pending_credentials_load(WifiCredentials *credentials)
{
    nvs_handle_t handle;
    if (nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    size_t ssid_length = sizeof(credentials->ssid);
    size_t password_length = sizeof(credentials->password);
    const esp_err_t ssid_result = nvs_get_str(handle, WIFI_PENDING_SSID_KEY, credentials->ssid, &ssid_length);
    const esp_err_t password_result = nvs_get_str(handle, WIFI_PENDING_PASSWORD_KEY, credentials->password, &password_length);
    nvs_close(handle);

    return ssid_result == ESP_OK && password_result == ESP_OK && credentials->ssid[0] != '\0';
}

static esp_err_t wifi_credentials_save(const WifiCredentials *credentials)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK)
    {
        return result;
    }

    result = nvs_set_str(handle, WIFI_SSID_KEY, credentials->ssid);
    if (result == ESP_OK)
    {
        result = nvs_set_str(handle, WIFI_PASSWORD_KEY, credentials->password);
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    return result;
}

static esp_err_t wifi_pending_credentials_save(const WifiCredentials *credentials)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK)
    {
        return result;
    }

    result = nvs_set_str(handle, WIFI_PENDING_SSID_KEY, credentials->ssid);
    if (result == ESP_OK)
    {
        result = nvs_set_str(handle, WIFI_PENDING_PASSWORD_KEY, credentials->password);
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    return result;
}

static esp_err_t wifi_pending_credentials_erase(void)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK)
    {
        return result;
    }

    result = nvs_erase_key(handle, WIFI_PENDING_SSID_KEY);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        result = ESP_OK;
    }
    if (result == ESP_OK)
    {
        result = nvs_erase_key(handle, WIFI_PENDING_PASSWORD_KEY);
        if (result == ESP_ERR_NVS_NOT_FOUND)
        {
            result = ESP_OK;
        }
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    return result;
}

static esp_err_t wifi_credentials_erase(void)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
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
    return result;
}

static void wifi_reset_credentials_if_requested(void)
{
    const gpio_config_t button_configuration = {
        .pin_bit_mask = 1ULL << WIFI_BOOT_BUTTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&button_configuration));

    if (gpio_get_level(WIFI_BOOT_BUTTON) != 0)
    {
        return;
    }

    ESP_LOGW(TAG, "BOOT held; keep holding for three seconds to erase saved Wi-Fi credentials");
    vTaskDelay(pdMS_TO_TICKS(WIFI_BOOT_RESET_HOLD_MS));
    if (gpio_get_level(WIFI_BOOT_BUTTON) != 0)
    {
        return;
    }

    ESP_ERROR_CHECK(wifi_credentials_erase());
    ESP_LOGW(TAG, "Saved Wi-Fi credentials erased; release BOOT now or keep holding for factory reset at fifteen seconds");

    vTaskDelay(pdMS_TO_TICKS(WIFI_BOOT_FACTORY_RESET_HOLD_MS - WIFI_BOOT_RESET_HOLD_MS));
    if (gpio_get_level(WIFI_BOOT_BUTTON) == 0)
    {
        ESP_ERROR_CHECK(management_factory_reset());
        ESP_LOGW(TAG, "Factory reset complete; Wi-Fi, ADMIN credentials, API credentials, and device HTTPS identity were erased");
    }
}

static wifi_config_t wifi_station_configuration(const WifiCredentials *credentials)
{
    wifi_config_t configuration = {0};
    const size_t ssid_length = strnlen(credentials->ssid, sizeof(credentials->ssid));
    const size_t password_length = strnlen(credentials->password, sizeof(credentials->password));
    memcpy(configuration.sta.ssid, credentials->ssid, ssid_length);
    memcpy(configuration.sta.password, credentials->password, password_length);
    configuration.sta.threshold.authmode = password_length == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    configuration.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    return configuration;
}

static void wifi_dhcp_start_if_needed(void)
{
    esp_netif_dhcp_status_t status;
    esp_err_t result = esp_netif_dhcpc_get_status(station_network_interface, &status);
    if (result != ESP_OK)
    {
        ESP_LOGW(TAG, "Unable to read DHCP client status: %s", esp_err_to_name(result));
        return;
    }

    ESP_LOGI(TAG, "Station associated; DHCP client status is %d", status);
    if (status != ESP_NETIF_DHCP_INIT && status != ESP_NETIF_DHCP_STOPPED)
    {
        return;
    }

    result = esp_netif_dhcpc_start(station_network_interface);
    if (result != ESP_OK && result != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)
    {
        ESP_LOGW(TAG, "Unable to start the DHCP client: %s", esp_err_to_name(result));
    }
}

static void wifi_event_handler(void *argument, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        const ip_event_got_ip_t *event = event_data;
        connection_retry_count = 0;
        xEventGroupClearBits(wifi_event_group, WIFI_FAILED_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_set_connection_diagnostic("Wi-Fi connected and a DHCP address was assigned.");
        ESP_LOGI(TAG, "Connected with address " IPSTR, IP2STR(&event->ip_info.ip));
        const esp_err_t management_result = management_server_start();
        if (management_result != ESP_OK)
        {
            ESP_LOGE(TAG, "Unable to start HTTPS management after Wi-Fi connection: %s",
                     esp_err_to_name(management_result));
        }
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        station_associated = true;
        wifi_set_connection_diagnostic("Wi-Fi associated. Waiting for a DHCP address.");
        wifi_dhcp_start_if_needed();
        return;
    }

    if (event_base != WIFI_EVENT || event_id != WIFI_EVENT_STA_DISCONNECTED)
    {
        return;
    }

    const wifi_event_sta_disconnected_t *event = event_data;
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    if (!connection_requested)
    {
        return;
    }

    ESP_LOGW(TAG, "Wi-Fi disconnected while connecting (reason %d)", event->reason);
    char diagnostic[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
    snprintf(diagnostic, sizeof(diagnostic),
             "Wi-Fi disconnected before a usable IP address was assigned (reason %d).",
             event->reason);
    wifi_set_connection_diagnostic(diagnostic);

    if (connection_retry_count < WIFI_MAXIMUM_RETRIES)
    {
        connection_retry_count++;
        ESP_LOGI(TAG, "Wi-Fi connection failed; retry %u of %u",
                 connection_retry_count, WIFI_MAXIMUM_RETRIES);
        esp_wifi_connect();
        return;
    }

    connection_requested = false;
    xEventGroupSetBits(wifi_event_group, WIFI_FAILED_BIT);
    ESP_LOGW(TAG, "Wi-Fi is unavailable after %u retries", WIFI_MAXIMUM_RETRIES);
}

static void http_set_common_headers(httpd_req_t *request)
{
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
}

static esp_err_t http_send_json(httpd_req_t *request, const char *status,
                                const char *json)
{
    http_set_common_headers(request);
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request, json);
}

static esp_err_t portal_root_handler(httpd_req_t *request)
{
    http_set_common_headers(request);
    httpd_resp_set_hdr(request, "Content-Security-Policy",
                       "default-src 'self'; style-src 'unsafe-inline'; script-src 'unsafe-inline'; form-action 'self'");
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_send(request, wifi_portal_html, wifi_portal_html_length);
}

static esp_err_t portal_not_found_handler(httpd_req_t *request, httpd_err_code_t error)
{
    (void)error;
    http_set_common_headers(request);
    httpd_resp_set_status(request, "303 See Other");
    httpd_resp_set_hdr(request, "Location", "/");
    return httpd_resp_sendstr(request, "Continue to ESP32-NUT Wi-Fi setup");
}

static esp_err_t json_send_escaped_string(httpd_req_t *request, const char *value)
{
    if (httpd_resp_send_chunk(request, "\"", 1) != ESP_OK)
    {
        return ESP_FAIL;
    }

    for (const unsigned char *character = (const unsigned char *)value; *character != '\0'; character++)
    {
        char encoded[7];
        const char *chunk = (const char *)character;
        size_t chunk_length = 1;
        if (*character == '\\' || *character == '\"')
        {
            encoded[0] = '\\';
            encoded[1] = (char)*character;
            chunk = encoded;
            chunk_length = 2;
        }
        else if (*character < 0x20)
        {
            snprintf(encoded, sizeof(encoded), "\\u%04x", *character);
            chunk = encoded;
            chunk_length = 6;
        }

        if (httpd_resp_send_chunk(request, chunk, chunk_length) != ESP_OK)
        {
            return ESP_FAIL;
        }
    }

    return httpd_resp_send_chunk(request, "\"", 1);
}

static esp_err_t portal_networks_handler(httpd_req_t *request)
{
    connection_requested = false;
    esp_wifi_disconnect();

    esp_err_t result = esp_wifi_scan_start(NULL, true);
    if (result != ESP_OK)
    {
        ESP_LOGW(TAG, "Wi-Fi scan failed: %s", esp_err_to_name(result));
        return http_send_json(request, "503 Service Unavailable", "{\"message\":\"Wi-Fi scan failed\"}");
    }

    uint16_t access_point_count = 0;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_num(&access_point_count));
    if (access_point_count > WIFI_SCAN_RESULT_LIMIT)
    {
        access_point_count = WIFI_SCAN_RESULT_LIMIT;
    }

    wifi_ap_record_t *records = calloc(access_point_count, sizeof(*records));
    if (access_point_count > 0 && records == NULL)
    {
        return http_send_json(request, "500 Internal Server Error", "{\"message\":\"Out of memory\"}");
    }

    uint16_t records_returned = access_point_count;
    result = esp_wifi_scan_get_ap_records(&records_returned, records);
    if (result != ESP_OK)
    {
        free(records);
        return http_send_json(request, "503 Service Unavailable", "{\"message\":\"Unable to read scan results\"}");
    }

    http_set_common_headers(request);
    httpd_resp_set_type(request, "application/json");
    httpd_resp_send_chunk(request, "[", 1);
    unsigned int unique_count = 0;
    for (uint16_t index = 0; index < records_returned; index++)
    {
        char ssid[33] = {0};
        const size_t ssid_length = strnlen((const char *)records[index].ssid, sizeof(ssid) - 1);
        if (ssid_length == 0)
        {
            continue;
        }
        memcpy(ssid, records[index].ssid, ssid_length);

        bool duplicate = false;
        for (uint16_t previous = 0; previous < index; previous++)
        {
            if (strncmp((const char *)records[previous].ssid, ssid, sizeof(records[previous].ssid)) == 0)
            {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
        {
            continue;
        }

        if (unique_count++ > 0)
        {
            httpd_resp_send_chunk(request, ",", 1);
        }
        if (json_send_escaped_string(request, ssid) != ESP_OK)
        {
            free(records);
            return ESP_FAIL;
        }
    }
    free(records);
    httpd_resp_send_chunk(request, "]", 1);
    return httpd_resp_send_chunk(request, NULL, 0);
}

static esp_err_t portal_status_handler(httpd_req_t *request)
{
    char diagnostic[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
    wifi_get_connection_diagnostic(diagnostic, sizeof(diagnostic));

    http_set_common_headers(request);
    httpd_resp_set_type(request, "application/json");
    if (httpd_resp_send_chunk(request, "{\"message\":", HTTPD_RESP_USE_STRLEN) != ESP_OK ||
        json_send_escaped_string(request, diagnostic) != ESP_OK ||
        httpd_resp_send_chunk(request, "}", HTTPD_RESP_USE_STRLEN) != ESP_OK)
    {
        return ESP_FAIL;
    }
    return httpd_resp_send_chunk(request, NULL, 0);
}

static int hexadecimal_value(char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    character = (char)tolower((unsigned char)character);
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    return -1;
}

static bool url_decode(char *destination, size_t destination_size, const char *source)
{
    size_t output_length = 0;
    while (*source != '\0')
    {
        if (output_length + 1 >= destination_size)
        {
            return false;
        }

        if (*source == '+')
        {
            destination[output_length++] = ' ';
            source++;
        }
        else if (*source == '%' && source[1] != '\0' && source[2] != '\0')
        {
            const int high = hexadecimal_value(source[1]);
            const int low = hexadecimal_value(source[2]);
            if (high < 0 || low < 0)
            {
                return false;
            }
            destination[output_length++] = (char)((high << 4) | low);
            source += 3;
        }
        else
        {
            destination[output_length++] = *source++;
        }
    }
    destination[output_length] = '\0';
    return true;
}

static void wifi_restart_task(void *parameter)
{
    (void)parameter;
    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
}

static esp_err_t portal_configure_handler(httpd_req_t *request)
{
    if (request->content_len <= 0 || request->content_len >= WIFI_REQUEST_BODY_LIMIT)
    {
        return http_send_json(request, "400 Bad Request", "{\"message\":\"Invalid request\"}");
    }

    char request_body[WIFI_REQUEST_BODY_LIMIT];
    size_t received = 0;
    while (received < (size_t)request->content_len)
    {
        const int result = httpd_req_recv(request, request_body + received,
                                          (size_t)request->content_len - received);
        if (result <= 0)
        {
            return http_send_json(request, "400 Bad Request", "{\"message\":\"Incomplete request\"}");
        }
        received += (size_t)result;
    }
    request_body[received] = '\0';

    char encoded_ssid[97];
    char encoded_password[190];
    WifiCredentials credentials = {0};
    if (httpd_query_key_value(request_body, "ssid", encoded_ssid, sizeof(encoded_ssid)) != ESP_OK ||
        httpd_query_key_value(request_body, "password", encoded_password, sizeof(encoded_password)) != ESP_OK ||
        !url_decode(credentials.ssid, sizeof(credentials.ssid), encoded_ssid) ||
        !url_decode(credentials.password, sizeof(credentials.password), encoded_password))
    {
        return http_send_json(request, "400 Bad Request", "{\"message\":\"Invalid network name or password\"}");
    }

    const size_t ssid_length = strlen(credentials.ssid);
    const size_t password_length = strlen(credentials.password);
    if (ssid_length == 0 || ssid_length > 32 ||
        (password_length > 0 && (password_length < 8 || password_length > 63)))
    {
        return http_send_json(request, "400 Bad Request",
                              "{\"message\":\"Use a 1-32 character network name and an 8-63 character password, or leave the password blank for an open network.\"}");
    }

    ESP_LOGI(TAG, "Saving pending credentials for Wi-Fi network '%s'", credentials.ssid);
    const esp_err_t result = wifi_pending_credentials_save(&credentials);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to save pending Wi-Fi credentials: %s", esp_err_to_name(result));
        return http_send_json(request, "500 Internal Server Error",
                              "{\"message\":\"Unable to save Wi-Fi credentials. Try again.\"}");
    }

    if (xTaskCreate(wifi_restart_task, "wifi-restart", WIFI_RESTART_TASK_STACK_SIZE,
                    NULL, 5, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Unable to schedule Wi-Fi validation restart");
        return http_send_json(request, "500 Internal Server Error",
                              "{\"message\":\"Unable to restart for Wi-Fi validation. Try again.\"}");
    }

    return http_send_json(
        request, "200 OK",
        "{\"message\":\"Credentials saved. The device will restart and test Wi-Fi without the setup access point. They will be kept for automatic retries if the connection cannot be completed.\"}");
}

static httpd_handle_t portal_http_start(void)
{
    httpd_config_t configuration = HTTPD_DEFAULT_CONFIG();
    configuration.max_open_sockets = 4;
    configuration.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &configuration) != ESP_OK)
    {
        return NULL;
    }

    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = portal_root_handler,
    };
    const httpd_uri_t networks = {
        .uri = "/api/networks",
        .method = HTTP_GET,
        .handler = portal_networks_handler,
    };
    const httpd_uri_t configure = {
        .uri = "/api/configure",
        .method = HTTP_POST,
        .handler = portal_configure_handler,
    };
    const httpd_uri_t status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = portal_status_handler,
    };

    if (httpd_register_uri_handler(server, &root) != ESP_OK ||
        httpd_register_uri_handler(server, &networks) != ESP_OK ||
        httpd_register_uri_handler(server, &configure) != ESP_OK ||
        httpd_register_uri_handler(server, &status) != ESP_OK ||
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, portal_not_found_handler) != ESP_OK)
    {
        httpd_stop(server);
        return NULL;
    }

    return server;
}

static esp_err_t wifi_portal_start(void)
{
    taskENTER_CRITICAL(&wifi_state_lock);
    if (portal_active)
    {
        taskEXIT_CRITICAL(&wifi_state_lock);
        return ESP_OK;
    }
    taskEXIT_CRITICAL(&wifi_state_lock);

    uint8_t mac_address[6];
    ESP_RETURN_ON_ERROR(esp_read_mac(mac_address, ESP_MAC_WIFI_SOFTAP), TAG,
                        "Unable to read Wi-Fi MAC address");

    wifi_config_t access_point_configuration = {0};
    snprintf((char *)access_point_configuration.ap.ssid,
             sizeof(access_point_configuration.ap.ssid),
             "ESP32-NUT-%02X%02X%02X",
             mac_address[3], mac_address[4], mac_address[5]);
    access_point_configuration.ap.ssid_len = strlen((const char *)access_point_configuration.ap.ssid);
    access_point_configuration.ap.channel = WIFI_AP_CHANNEL;
    access_point_configuration.ap.max_connection = WIFI_AP_MAX_CONNECTIONS;
    access_point_configuration.ap.authmode = WIFI_AUTH_OPEN;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG,
                        "Unable to enable fallback access point mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &access_point_configuration), TAG,
                        "Unable to configure fallback access point");

    esp_netif_ip_info_t ip_info;
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(access_point_network_interface, &ip_info), TAG,
                        "Unable to read fallback access point address");

    static char captive_portal_url[32];
    snprintf(captive_portal_url, sizeof(captive_portal_url), "http://" IPSTR, IP2STR(&ip_info.ip));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(access_point_network_interface));
    ESP_RETURN_ON_ERROR(esp_netif_dhcps_option(access_point_network_interface,
                                               ESP_NETIF_OP_SET,
                                               ESP_NETIF_CAPTIVEPORTAL_URI,
                                               captive_portal_url,
                                               strlen(captive_portal_url)),
                        TAG, "Unable to advertise captive portal URL");
    ESP_RETURN_ON_ERROR(esp_netif_dhcps_start(access_point_network_interface), TAG,
                        "Unable to restart fallback DHCP server");

    portal_http_server = portal_http_start();
    ESP_RETURN_ON_FALSE(portal_http_server != NULL, ESP_FAIL, TAG,
                        "Unable to start captive portal web server");

    portal_dns_server = dns_server_start(WIFI_AP_INTERFACE_KEY);
    if (portal_dns_server == NULL)
    {
        httpd_stop(portal_http_server);
        portal_http_server = NULL;
        return ESP_FAIL;
    }

    taskENTER_CRITICAL(&wifi_state_lock);
    portal_active = true;
    portal_start_scheduled = false;
    taskEXIT_CRITICAL(&wifi_state_lock);
    xEventGroupSetBits(wifi_event_group, WIFI_PORTAL_STARTED_BIT);

    ESP_LOGW(TAG, "Open setup access point '%s' active at " IPSTR,
             access_point_configuration.ap.ssid, IP2STR(&ip_info.ip));
    ESP_LOGW(TAG, "The setup portal has no access-point password or portal authentication");
    return ESP_OK;
}

static void wifi_portal_start_task(void *parameter)
{
    (void)parameter;
    const esp_err_t result = wifi_portal_start();
    if (result != ESP_OK)
    {
        taskENTER_CRITICAL(&wifi_state_lock);
        portal_start_scheduled = false;
        taskEXIT_CRITICAL(&wifi_state_lock);
        ESP_LOGE(TAG, "Fallback setup portal failed to start: %s", esp_err_to_name(result));
    }
    vTaskDelete(NULL);
}

static void wifi_schedule_portal(void)
{
    bool should_start = false;
    taskENTER_CRITICAL(&wifi_state_lock);
    if (!portal_active && !portal_start_scheduled)
    {
        portal_start_scheduled = true;
        should_start = true;
    }
    taskEXIT_CRITICAL(&wifi_state_lock);

    if (should_start && xTaskCreate(wifi_portal_start_task, "wifi-portal",
                                    WIFI_PORTAL_TASK_STACK_SIZE, NULL, 5, NULL) != pdPASS)
    {
        taskENTER_CRITICAL(&wifi_state_lock);
        portal_start_scheduled = false;
        taskEXIT_CRITICAL(&wifi_state_lock);
        ESP_LOGE(TAG, "Unable to create fallback portal task");
    }
}

static bool wifi_connect_with_timeout(const WifiCredentials *credentials,
                                      TickType_t timeout)
{
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT);
    connection_retry_count = 0;
    connection_requested = false;
    station_associated = false;
    wifi_set_connection_diagnostic("Connecting to Wi-Fi.");

    wifi_config_t station_configuration = wifi_station_configuration(credentials);
    esp_err_t result = esp_wifi_set_config(WIFI_IF_STA, &station_configuration);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to configure Wi-Fi station: %s", esp_err_to_name(result));
        return false;
    }

    connection_requested = true;
    result = esp_wifi_connect();
    if (result != ESP_OK)
    {
        connection_requested = false;
        ESP_LOGE(TAG, "Unable to begin Wi-Fi connection: %s", esp_err_to_name(result));
        return false;
    }

    const EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                                 WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                                 pdFALSE, pdFALSE, timeout);
    if ((bits & WIFI_CONNECTED_BIT) != 0)
    {
        return true;
    }

    if (station_associated)
    {
        wifi_capture_dhcp_diagnostic();
    }
    else if ((bits & WIFI_FAILED_BIT) == 0)
    {
        wifi_set_connection_diagnostic("Wi-Fi association did not complete before the connection timed out.");
    }

    connection_requested = false;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
    return false;
}

void wifi_provisioning_init(void)
{
    nvs_initialize();
    wifi_reset_credentials_if_requested();

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    station_network_interface = esp_netif_create_default_wifi_sta();
    access_point_network_interface = esp_netif_create_default_wifi_ap();
    assert(station_network_interface != NULL && access_point_network_interface != NULL);

    wifi_init_config_t initialization = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&initialization));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    WifiCredentials credentials = {0};
    if (wifi_pending_credentials_load(&credentials))
    {
        ESP_LOGI(TAG, "Testing pending Wi-Fi credentials for '%s' in station-only mode",
                 credentials.ssid);
        if (wifi_connect_with_timeout(&credentials,
                                      pdMS_TO_TICKS(WIFI_PENDING_CONNECT_TIMEOUT_MS)))
        {
            const esp_err_t save_result = wifi_credentials_save(&credentials);
            const esp_err_t erase_result = wifi_pending_credentials_erase();
            if (save_result == ESP_OK)
            {
                if (erase_result != ESP_OK)
                {
                    ESP_LOGW(TAG, "Wi-Fi credentials saved but pending state could not be erased: %s",
                             esp_err_to_name(erase_result));
                }
                else
                {
                    ESP_LOGI(TAG, "Pending Wi-Fi credentials validated and saved");
                }
                return;
            }

            ESP_LOGE(TAG, "Wi-Fi connected but credentials could not be saved: %s",
                     esp_err_to_name(save_result));
        }

        connection_requested = false;
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
        ESP_LOGW(TAG, "Pending Wi-Fi connection did not complete; retaining credentials for automatic retries");
    }
    else if (wifi_credentials_load(&credentials))
    {
        ESP_LOGI(TAG, "Connecting to saved Wi-Fi network '%s'", credentials.ssid);
        if (wifi_connect_with_timeout(&credentials,
                                      pdMS_TO_TICKS(WIFI_SAVED_CONNECT_TIMEOUT_MS)))
        {
            ESP_LOGI(TAG, "Saved Wi-Fi connection established");
            return;
        }

        ESP_LOGW(TAG, "Saved Wi-Fi connection did not complete; starting setup mode");
    }
    else
    {
        wifi_set_connection_diagnostic("No saved Wi-Fi credentials. Select a network and connect.");
        ESP_LOGI(TAG, "No saved Wi-Fi credentials; starting setup mode");
    }

    wifi_schedule_portal();
    const EventBits_t portal_bits = xEventGroupWaitBits(wifi_event_group,
                                                        WIFI_PORTAL_STARTED_BIT,
                                                        pdFALSE, pdFALSE,
                                                        pdMS_TO_TICKS(WIFI_PORTAL_START_TIMEOUT_MS));
    if ((portal_bits & WIFI_PORTAL_STARTED_BIT) == 0)
    {
        ESP_LOGE(TAG, "Fallback setup portal did not start");
    }
}

bool wifi_provisioning_is_connected(void)
{
    return wifi_event_group != NULL &&
           (xEventGroupGetBits(wifi_event_group) & WIFI_CONNECTED_BIT) != 0;
}
