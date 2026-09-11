/** @file wifi-portal-lifecycle.c @brief Start and schedule the fallback Wi-Fi setup portal. */
#include "wifi-portal-lifecycle.h"

#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"

#define TAG "nut-wifi"
#define WIFI_AP_INTERFACE_KEY "WIFI_AP_DEF"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONNECTIONS 4
#define WIFI_PORTAL_TASK_STACK_SIZE 4096
#define WIFI_PORTAL_STARTED_BIT BIT2

static esp_err_t wifi_portal_lifecycle_start(WifiPortalLifecycle *lifecycle)
{
    taskENTER_CRITICAL(lifecycle->state_lock);
    if (*lifecycle->portal_active)
    {
        taskEXIT_CRITICAL(lifecycle->state_lock);
        return ESP_OK;
    }
    taskEXIT_CRITICAL(lifecycle->state_lock);
    uint8_t mac_address[6];
    ESP_RETURN_ON_ERROR(esp_read_mac(mac_address, ESP_MAC_WIFI_SOFTAP), TAG,
                        "Unable to read Wi-Fi MAC address");
    wifi_config_t configuration = {0};
    snprintf((char *)configuration.ap.ssid, sizeof(configuration.ap.ssid),
             "ESP32-NUT-%02X%02X%02X", mac_address[3], mac_address[4], mac_address[5]);
    configuration.ap.ssid_len = strlen((const char *)configuration.ap.ssid);
    configuration.ap.channel = WIFI_AP_CHANNEL;
    configuration.ap.max_connection = WIFI_AP_MAX_CONNECTIONS;
    configuration.ap.authmode = WIFI_AUTH_OPEN;
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG,
                        "Unable to enable fallback access point mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &configuration), TAG,
                        "Unable to configure fallback access point");
    esp_netif_ip_info_t ip_info;
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(lifecycle->access_point_network_interface, &ip_info),
                        TAG, "Unable to read fallback access point address");
    static char captive_portal_url[32];
    snprintf(captive_portal_url, sizeof(captive_portal_url), "http://" IPSTR, IP2STR(&ip_info.ip));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(lifecycle->access_point_network_interface));
    ESP_RETURN_ON_ERROR(esp_netif_dhcps_option(lifecycle->access_point_network_interface,
                                               ESP_NETIF_OP_SET,
                                               ESP_NETIF_CAPTIVEPORTAL_URI,
                                               captive_portal_url,
                                               strlen(captive_portal_url)),
                        TAG, "Unable to advertise captive portal URL");
    ESP_RETURN_ON_ERROR(esp_netif_dhcps_start(lifecycle->access_point_network_interface), TAG,
                        "Unable to restart fallback DHCP server");
    *lifecycle->portal_http_server = wifi_provisioning_web_start(lifecycle->web_context);
    ESP_RETURN_ON_FALSE(*lifecycle->portal_http_server != NULL, ESP_FAIL, TAG,
                        "Unable to start captive portal web server");
    *lifecycle->portal_dns_server = dns_server_start(WIFI_AP_INTERFACE_KEY);
    if (*lifecycle->portal_dns_server == NULL)
    {
        httpd_stop(*lifecycle->portal_http_server);
        *lifecycle->portal_http_server = NULL;
        return ESP_FAIL;
    }
    taskENTER_CRITICAL(lifecycle->state_lock);
    *lifecycle->portal_active = true;
    *lifecycle->portal_start_scheduled = false;
    taskEXIT_CRITICAL(lifecycle->state_lock);
    xEventGroupSetBits(lifecycle->event_group, WIFI_PORTAL_STARTED_BIT);
    ESP_LOGW(TAG, "Open setup access point '%s' active at " IPSTR,
             configuration.ap.ssid, IP2STR(&ip_info.ip));
    ESP_LOGW(TAG, "The setup portal has no access-point password or portal authentication");
    return ESP_OK;
}

static void wifi_portal_lifecycle_task(void *parameter)
{
    WifiPortalLifecycle *lifecycle = parameter;
    const esp_err_t result = wifi_portal_lifecycle_start(lifecycle);
    if (result != ESP_OK)
    {
        taskENTER_CRITICAL(lifecycle->state_lock);
        *lifecycle->portal_start_scheduled = false;
        taskEXIT_CRITICAL(lifecycle->state_lock);
        ESP_LOGE(TAG, "Fallback setup portal failed to start: %s", esp_err_to_name(result));
    }
    vTaskDelete(NULL);
}

void wifi_portal_lifecycle_schedule(WifiPortalLifecycle *lifecycle)
{
    bool should_start = false;
    taskENTER_CRITICAL(lifecycle->state_lock);
    if (!*lifecycle->portal_active && !*lifecycle->portal_start_scheduled)
    {
        *lifecycle->portal_start_scheduled = true;
        should_start = true;
    }
    taskEXIT_CRITICAL(lifecycle->state_lock);
    if (should_start &&
        xTaskCreate(wifi_portal_lifecycle_task, "wifi-portal", WIFI_PORTAL_TASK_STACK_SIZE,
                    lifecycle, 5, NULL) != pdPASS)
    {
        taskENTER_CRITICAL(lifecycle->state_lock);
        *lifecycle->portal_start_scheduled = false;
        taskEXIT_CRITICAL(lifecycle->state_lock);
        ESP_LOGE(TAG, "Unable to create fallback portal task");
    }
}
