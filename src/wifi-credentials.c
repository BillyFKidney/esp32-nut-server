/** @file wifi-credentials.c @brief Persist active and pending Wi-Fi credentials. @see wifi-credentials.h, nvs.h */
#include "wifi-credentials.h"

#include "nvs.h"

#define WIFI_CONFIG_NAMESPACE "wifi-config"
#define WIFI_SSID_KEY "ssid"
#define WIFI_PASSWORD_KEY "password"
#define WIFI_PENDING_SSID_KEY "pending-ssid"
#define WIFI_PENDING_PASSWORD_KEY "pending-pass"

typedef struct
{
    const char *ssid_key;
    const char *password_key;
} WifiCredentialsKeys;

static const WifiCredentialsKeys active_credentials_keys = {
    .ssid_key = WIFI_SSID_KEY,
    .password_key = WIFI_PASSWORD_KEY,
};

static const WifiCredentialsKeys pending_credentials_keys = {
    .ssid_key = WIFI_PENDING_SSID_KEY,
    .password_key = WIFI_PENDING_PASSWORD_KEY,
};

static bool wifi_credentials_load_keys(WifiCredentials *credentials,
                                       const WifiCredentialsKeys *keys)
{
    nvs_handle_t handle;
    if (nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    size_t ssid_length = sizeof(credentials->ssid);
    size_t password_length = sizeof(credentials->password);
    const esp_err_t ssid_result =
        nvs_get_str(handle, keys->ssid_key, credentials->ssid, &ssid_length);
    const esp_err_t password_result =
        nvs_get_str(handle, keys->password_key, credentials->password, &password_length);
    nvs_close(handle);

    return ssid_result == ESP_OK && password_result == ESP_OK && credentials->ssid[0] != '\0';
}

bool wifi_credentials_load(WifiCredentials *credentials)
{
    return wifi_credentials_load_keys(credentials, &active_credentials_keys);
}

bool wifi_pending_credentials_load(WifiCredentials *credentials)
{
    return wifi_credentials_load_keys(credentials, &pending_credentials_keys);
}

static esp_err_t wifi_credentials_save_keys(const WifiCredentials *credentials,
                                            const WifiCredentialsKeys *keys)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK)
    {
        return result;
    }

    result = nvs_set_str(handle, keys->ssid_key, credentials->ssid);
    if (result == ESP_OK)
    {
        result = nvs_set_str(handle, keys->password_key, credentials->password);
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    return result;
}

esp_err_t wifi_credentials_save(const WifiCredentials *credentials)
{
    return wifi_credentials_save_keys(credentials, &active_credentials_keys);
}

esp_err_t wifi_pending_credentials_save(const WifiCredentials *credentials)
{
    return wifi_credentials_save_keys(credentials, &pending_credentials_keys);
}

esp_err_t wifi_pending_credentials_erase(void)
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

esp_err_t wifi_credentials_erase(void)
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
