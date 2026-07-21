#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#define WIFI_MANAGEMENT_SCAN_RESULT_LIMIT 20U
#define WIFI_MANAGEMENT_SSID_MAX_LENGTH 32U
#define WIFI_MANAGEMENT_PASSWORD_MAX_LENGTH 63U

typedef struct
{
    char ssid[WIFI_MANAGEMENT_SSID_MAX_LENGTH + 1U];
    int8_t rssi_dbm;
    uint8_t authmode;
} WifiManagementScanResult;

typedef struct
{
    size_t count;
    WifiManagementScanResult entries[WIFI_MANAGEMENT_SCAN_RESULT_LIMIT];
} WifiManagementScanResults;

/**
 * Connect to saved Wi-Fi or start the open fallback provisioning portal.
 *
 * This function initializes NVS, esp-netif, the default event loop, and Wi-Fi.
 */
void wifi_provisioning_init(void);

/** Return true when the station interface has obtained an IPv4 address. */
bool wifi_provisioning_is_connected(void);

/** Scan visible 2.4 GHz networks for the authenticated management console. */
esp_err_t wifi_management_scan(WifiManagementScanResults *results);

/** Stage credentials for a safe reboot-and-validate reconnect. */
esp_err_t wifi_management_stage_credentials(const char *ssid, const char *password);
