#pragma once

#include <stdbool.h>

#include "esp_err.h"

#define WIFI_CREDENTIALS_SSID_CAPACITY 33U
#define WIFI_CREDENTIALS_PASSWORD_CAPACITY 64U

typedef struct
{
    char ssid[WIFI_CREDENTIALS_SSID_CAPACITY];
    char password[WIFI_CREDENTIALS_PASSWORD_CAPACITY];
} WifiCredentials;

/** Load the currently active Wi-Fi credentials from the wifi-config namespace. */
bool wifi_credentials_load(WifiCredentials *credentials);

/** Load the one-time pending Wi-Fi credentials from the wifi-config namespace. */
bool wifi_pending_credentials_load(WifiCredentials *credentials);

/** Persist a validated active Wi-Fi credential record. */
esp_err_t wifi_credentials_save(const WifiCredentials *credentials);

/** Persist a one-time Wi-Fi credential record for boot-time validation. */
esp_err_t wifi_pending_credentials_save(const WifiCredentials *credentials);

/** Erase only the pending Wi-Fi credential record. */
esp_err_t wifi_pending_credentials_erase(void);

/** Erase the entire wifi-config namespace, including active and pending records. */
esp_err_t wifi_credentials_erase(void);
