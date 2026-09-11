#pragma once

#include <stdbool.h>

#include "dns-server.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "wifi-provisioning-web.h"

typedef struct
{
    EventGroupHandle_t event_group;
    esp_netif_t *access_point_network_interface;
    httpd_handle_t *portal_http_server;
    DnsServerHandle *portal_dns_server;
    bool *portal_active;
    bool *portal_start_scheduled;
    portMUX_TYPE *state_lock;
    WifiProvisioningWebContext *web_context;
} WifiPortalLifecycle;

void wifi_portal_lifecycle_schedule(WifiPortalLifecycle *lifecycle);
