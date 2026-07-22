#include "management.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "api_tokens.h"
#include "drivers/dstate.h"
#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "mbedtls/pk.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/x509_crt.h"
#include "nvs.h"
#include "ota.h"
#include "psa/crypto.h"
#include "time_config.h"
#include "wifi-provisioning.h"

#define TAG "nut-management"

#define MANAGEMENT_NAMESPACE "management"
#define MANAGEMENT_ADMIN_SALT_KEY "admin-salt"
#define MANAGEMENT_ADMIN_HASH_KEY "admin-hash"
#define MANAGEMENT_ADMIN_CREDENTIAL_KEY "admin-cred"
#define MANAGEMENT_CERTIFICATE_KEY "https-cert"
#define MANAGEMENT_PRIVATE_KEY_KEY "https-key"
#define MANAGEMENT_DEVICE_NAME_KEY "device-name"

#define MANAGEMENT_DEFAULT_DEVICE_NAME "ESP32-NUT"
#define MANAGEMENT_HTTPS_PORT 443
#define MANAGEMENT_PASSWORD_SALT_BYTES 16
#define MANAGEMENT_PASSWORD_HASH_BYTES 32
#define MANAGEMENT_PASSWORD_CREDENTIAL_VERSION 1U
#define MANAGEMENT_PASSWORD_ITERATIONS 12500U
#define MANAGEMENT_PASSWORD_LEGACY_ITERATIONS 100000U
#define MANAGEMENT_PASSWORD_MIN_ITERATIONS 1000U
#define MANAGEMENT_PASSWORD_MAX_ITERATIONS 1000000U
#define MANAGEMENT_SESSION_BYTES 32
#define MANAGEMENT_SESSION_HEX_LENGTH (MANAGEMENT_SESSION_BYTES * 2)
#define MANAGEMENT_SESSION_IDLE_US (15LL * 60LL * 1000000LL)
#define MANAGEMENT_FORM_BODY_LIMIT 640
#define MANAGEMENT_ADMIN_PAGE_SIZE 28000
#define MANAGEMENT_STATUS_RESPONSE_SIZE 5000
#define MANAGEMENT_WIFI_SCAN_RESPONSE_SIZE 4200
#define MANAGEMENT_NUT_VALUE_LENGTH 96
#define MANAGEMENT_HTTPS_ROUTE_CAPACITY 16
#define MANAGEMENT_CERTIFICATE_BUFFER_SIZE 2048
#define MANAGEMENT_PRIVATE_KEY_BUFFER_SIZE 1024
#define MANAGEMENT_LOGIN_MAX_FAILURES 5
#define MANAGEMENT_LOGIN_COOLDOWN_US (60LL * 1000000LL)

_Static_assert(sizeof(MANAGEMENT_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "Management NVS namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_ADMIN_SALT_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "ADMIN salt NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_ADMIN_HASH_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "ADMIN hash NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_ADMIN_CREDENTIAL_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "ADMIN credential NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_CERTIFICATE_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "HTTPS certificate NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_PRIVATE_KEY_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "HTTPS private-key NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_DEVICE_NAME_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "Device-name NVS key exceeds the ESP-IDF limit");

typedef struct
{
    bool active;
    char cookie[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    int64_t last_activity_us;
} ManagementSession;

typedef struct
{
    uint32_t version;
    uint32_t iterations;
    uint8_t salt[MANAGEMENT_PASSWORD_SALT_BYTES];
    uint8_t hash[MANAGEMENT_PASSWORD_HASH_BYTES];
} ManagementAdminCredential;

static httpd_handle_t management_https_server;
static uint8_t *management_certificate;
static size_t management_certificate_length;
static uint8_t *management_private_key;
static size_t management_private_key_length;
static ManagementSession management_session;
static portMUX_TYPE management_session_lock = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE management_login_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned int management_login_failures;
static int64_t management_login_cooldown_until_us;

extern const char *upsname;

static void management_set_response_headers(httpd_req_t *request)
{
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "Pragma", "no-cache");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(request, "X-Frame-Options", "DENY");
    httpd_resp_set_hdr(request, "Referrer-Policy", "no-referrer");
}

static esp_err_t management_send_html(httpd_req_t *request, const char *html)
{
    management_set_response_headers(request);
    httpd_resp_set_hdr(request, "Content-Security-Policy",
                       "default-src 'self'; style-src 'unsafe-inline'; script-src 'unsafe-inline'; base-uri 'none'; frame-ancestors 'none'; form-action 'self'");
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_sendstr(request, html);
}

static esp_err_t management_send_html_status(httpd_req_t *request, const char *status,
                                             const char *html)
{
    httpd_resp_set_status(request, status);
    return management_send_html(request, html);
}

static esp_err_t management_send_json(httpd_req_t *request, const char *status,
                                      const char *json)
{
    management_set_response_headers(request);
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request, json);
}

static esp_err_t management_send_redirect(httpd_req_t *request, const char *location)
{
    management_set_response_headers(request);
    httpd_resp_set_status(request, "303 See Other");
    httpd_resp_set_hdr(request, "Location", location);
    return httpd_resp_sendstr(request, "Continue");
}

static void management_bytes_to_hex(const uint8_t *source, size_t source_length,
                                    char *destination, size_t destination_length)
{
    static const char hexadecimal[] = "0123456789abcdef";
    if (destination_length < source_length * 2 + 1)
    {
        if (destination_length > 0)
        {
            destination[0] = '\0';
        }
        return;
    }

    for (size_t index = 0; index < source_length; index++)
    {
        destination[index * 2] = hexadecimal[source[index] >> 4];
        destination[index * 2 + 1] = hexadecimal[source[index] & 0x0f];
    }
    destination[source_length * 2] = '\0';
}

static bool management_constant_time_equal(const uint8_t *left, const uint8_t *right,
                                           size_t length)
{
    uint8_t difference = 0;
    for (size_t index = 0; index < length; index++)
    {
        difference |= left[index] ^ right[index];
    }
    return difference == 0;
}

static bool management_json_append(char *destination, size_t destination_size,
                                   size_t *used, const char *format, ...)
{
    if (destination == NULL || used == NULL || *used >= destination_size)
    {
        return false;
    }

    va_list arguments;
    va_start(arguments, format);
    const int written = vsnprintf(destination + *used,
                                  destination_size - *used, format, arguments);
    va_end(arguments);
    if (written < 0 || (size_t)written >= destination_size - *used)
    {
        *used = destination_size;
        return false;
    }
    *used += (size_t)written;
    return true;
}

static bool management_json_append_string(char *destination, size_t destination_size,
                                          size_t *used, const char *value)
{
    if (!management_json_append(destination, destination_size, used, "\""))
    {
        return false;
    }

    if (value == NULL)
    {
        value = "";
    }
    for (const unsigned char *cursor = (const unsigned char *)value; *cursor != '\0'; cursor++)
    {
        switch (*cursor)
        {
        case '\\':
            if (!management_json_append(destination, destination_size, used, "\\\\"))
            {
                return false;
            }
            break;
        case '"':
            if (!management_json_append(destination, destination_size, used, "\\\""))
            {
                return false;
            }
            break;
        case '\b':
            if (!management_json_append(destination, destination_size, used, "\\b"))
            {
                return false;
            }
            break;
        case '\f':
            if (!management_json_append(destination, destination_size, used, "\\f"))
            {
                return false;
            }
            break;
        case '\n':
            if (!management_json_append(destination, destination_size, used, "\\n"))
            {
                return false;
            }
            break;
        case '\r':
            if (!management_json_append(destination, destination_size, used, "\\r"))
            {
                return false;
            }
            break;
        case '\t':
            if (!management_json_append(destination, destination_size, used, "\\t"))
            {
                return false;
            }
            break;
        default:
            if (*cursor < 0x20U &&
                !management_json_append(destination, destination_size, used,
                                         "\\u%04x", (unsigned int)*cursor))
            {
                return false;
            }
            else if (*cursor >= 0x20U &&
                     !management_json_append(destination, destination_size, used,
                                              "%c", (char)*cursor))
            {
                return false;
            }
            break;
        }
    }
    return management_json_append(destination, destination_size, used, "\"");
}

typedef struct
{
    bool available;
    bool stale;
    char ups_name[32];
    char manufacturer[MANAGEMENT_NUT_VALUE_LENGTH];
    char model[MANAGEMENT_NUT_VALUE_LENGTH];
    char serial[MANAGEMENT_NUT_VALUE_LENGTH];
    char status[MANAGEMENT_NUT_VALUE_LENGTH];
    char battery_charge[MANAGEMENT_NUT_VALUE_LENGTH];
    char battery_runtime[MANAGEMENT_NUT_VALUE_LENGTH];
    char battery_voltage[MANAGEMENT_NUT_VALUE_LENGTH];
    char load[MANAGEMENT_NUT_VALUE_LENGTH];
    char input_voltage[MANAGEMENT_NUT_VALUE_LENGTH];
    char output_voltage[MANAGEMENT_NUT_VALUE_LENGTH];
    char ups_power[MANAGEMENT_NUT_VALUE_LENGTH];
    char ups_realpower[MANAGEMENT_NUT_VALUE_LENGTH];
    char ups_firmware[MANAGEMENT_NUT_VALUE_LENGTH];
} ManagementNutSnapshot;

static void management_copy_nut_value(const char *name, char *destination,
                                      size_t destination_size)
{
    const char *value = dstate_getinfo(name);
    if (value == NULL || *value == '\0')
    {
        snprintf(destination, destination_size, "unavailable");
        return;
    }
    snprintf(destination, destination_size, "%s", value);
}

static void management_collect_nut_snapshot(ManagementNutSnapshot *snapshot)
{
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->stale = dstate_is_stale() != 0;
    const char *status = dstate_getinfo("ups.status");
    snapshot->available = status != NULL && !snapshot->stale;
    snprintf(snapshot->ups_name, sizeof(snapshot->ups_name), "%s",
             upsname != NULL && *upsname != '\0' ? upsname : "cyberpower");
    management_copy_nut_value("device.mfr", snapshot->manufacturer,
                              sizeof(snapshot->manufacturer));
    if (strcmp(snapshot->manufacturer, "unavailable") == 0)
    {
        management_copy_nut_value("ups.mfr", snapshot->manufacturer,
                                  sizeof(snapshot->manufacturer));
    }
    management_copy_nut_value("device.model", snapshot->model,
                              sizeof(snapshot->model));
    if (strcmp(snapshot->model, "unavailable") == 0)
    {
        management_copy_nut_value("ups.model", snapshot->model,
                                  sizeof(snapshot->model));
    }
    management_copy_nut_value("device.serial", snapshot->serial,
                              sizeof(snapshot->serial));
    if (strcmp(snapshot->serial, "unavailable") == 0)
    {
        management_copy_nut_value("ups.serial", snapshot->serial,
                                  sizeof(snapshot->serial));
    }
    management_copy_nut_value("ups.status", snapshot->status,
                              sizeof(snapshot->status));
    management_copy_nut_value("battery.charge", snapshot->battery_charge,
                              sizeof(snapshot->battery_charge));
    management_copy_nut_value("battery.runtime", snapshot->battery_runtime,
                              sizeof(snapshot->battery_runtime));
    management_copy_nut_value("battery.voltage", snapshot->battery_voltage,
                              sizeof(snapshot->battery_voltage));
    management_copy_nut_value("ups.load", snapshot->load,
                              sizeof(snapshot->load));
    management_copy_nut_value("input.voltage", snapshot->input_voltage,
                              sizeof(snapshot->input_voltage));
    management_copy_nut_value("output.voltage", snapshot->output_voltage,
                              sizeof(snapshot->output_voltage));
    management_copy_nut_value("ups.power", snapshot->ups_power,
                              sizeof(snapshot->ups_power));
    management_copy_nut_value("ups.realpower", snapshot->ups_realpower,
                              sizeof(snapshot->ups_realpower));
    management_copy_nut_value("ups.firmware", snapshot->ups_firmware,
                              sizeof(snapshot->ups_firmware));
}

static esp_err_t management_open_nvs(nvs_open_mode_t mode, nvs_handle_t *handle)
{
    return nvs_open(MANAGEMENT_NAMESPACE, mode, handle);
}

bool management_admin_password_is_configured(void)
{
    nvs_handle_t handle = 0;
    if (management_open_nvs(NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    ManagementAdminCredential credential = {0};
    size_t credential_length = sizeof(credential);
    const esp_err_t credential_result = nvs_get_blob(handle,
                                                     MANAGEMENT_ADMIN_CREDENTIAL_KEY,
                                                     &credential,
                                                     &credential_length);
    if (credential_result == ESP_OK)
    {
        nvs_close(handle);
        const bool valid = credential_length == sizeof(credential) &&
                           credential.version == MANAGEMENT_PASSWORD_CREDENTIAL_VERSION &&
                           credential.iterations >= MANAGEMENT_PASSWORD_MIN_ITERATIONS &&
                           credential.iterations <= MANAGEMENT_PASSWORD_MAX_ITERATIONS;
        mbedtls_platform_zeroize(&credential, sizeof(credential));
        return valid;
    }
    mbedtls_platform_zeroize(&credential, sizeof(credential));
    if (credential_result != ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(handle);
        return false;
    }

    size_t salt_length = 0;
    size_t hash_length = 0;
    const esp_err_t salt_result = nvs_get_blob(handle, MANAGEMENT_ADMIN_SALT_KEY,
                                               NULL, &salt_length);
    const esp_err_t hash_result = nvs_get_blob(handle, MANAGEMENT_ADMIN_HASH_KEY,
                                               NULL, &hash_length);
    nvs_close(handle);
    return salt_result == ESP_OK && hash_result == ESP_OK &&
           salt_length == MANAGEMENT_PASSWORD_SALT_BYTES &&
           hash_length == MANAGEMENT_PASSWORD_HASH_BYTES;
}

static esp_err_t management_derive_password_hash(const char *password,
                                                  const uint8_t *salt,
                                                  uint32_t iterations,
                                                  uint8_t *hash)
{
    psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;
    psa_status_t result = psa_crypto_init();
    if (result == PSA_SUCCESS)
    {
        result = psa_key_derivation_setup(&operation,
                                          PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256));
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_key_derivation_input_integer(&operation,
                                                  PSA_KEY_DERIVATION_INPUT_COST,
                                                  iterations);
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_key_derivation_input_bytes(&operation,
                                                PSA_KEY_DERIVATION_INPUT_SALT,
                                                salt, MANAGEMENT_PASSWORD_SALT_BYTES);
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_key_derivation_input_bytes(&operation,
                                                PSA_KEY_DERIVATION_INPUT_PASSWORD,
                                                (const uint8_t *)password,
                                                strlen(password));
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_key_derivation_output_bytes(&operation, hash,
                                                 MANAGEMENT_PASSWORD_HASH_BYTES);
    }
    psa_key_derivation_abort(&operation);
    return result == PSA_SUCCESS ? ESP_OK : ESP_FAIL;
}

static esp_err_t management_set_admin_password(const char *password)
{
    if (password == NULL || strlen(password) < 12 || strlen(password) > 128)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ManagementAdminCredential credential = {
        .version = MANAGEMENT_PASSWORD_CREDENTIAL_VERSION,
        .iterations = MANAGEMENT_PASSWORD_ITERATIONS,
    };
    esp_fill_random(credential.salt, sizeof(credential.salt));
    esp_err_t result = management_derive_password_hash(password, credential.salt,
                                                       credential.iterations,
                                                       credential.hash);
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(&credential, sizeof(credential));
        return result;
    }

    nvs_handle_t handle = 0;
    result = management_open_nvs(NVS_READWRITE, &handle);
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, MANAGEMENT_ADMIN_CREDENTIAL_KEY,
                              &credential, sizeof(credential));
    }
    if (result == ESP_OK)
    {
        const esp_err_t erase_result = nvs_erase_key(handle, MANAGEMENT_ADMIN_SALT_KEY);
        if (erase_result != ESP_OK && erase_result != ESP_ERR_NVS_NOT_FOUND)
        {
            result = erase_result;
        }
    }
    if (result == ESP_OK)
    {
        const esp_err_t erase_result = nvs_erase_key(handle, MANAGEMENT_ADMIN_HASH_KEY);
        if (erase_result != ESP_OK && erase_result != ESP_ERR_NVS_NOT_FOUND)
        {
            result = erase_result;
        }
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    if (handle != 0)
    {
        nvs_close(handle);
    }
    mbedtls_platform_zeroize(&credential, sizeof(credential));
    return result;
}

static bool management_verify_admin_password(const char *password, bool *needs_migration)
{
    uint8_t salt[MANAGEMENT_PASSWORD_SALT_BYTES] = {0};
    uint8_t stored_hash[MANAGEMENT_PASSWORD_HASH_BYTES] = {0};
    uint8_t candidate_hash[MANAGEMENT_PASSWORD_HASH_BYTES] = {0};
    uint32_t iterations = 0;
    nvs_handle_t handle = 0;
    if (needs_migration != NULL)
    {
        *needs_migration = false;
    }
    if (password == NULL || management_open_nvs(NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    ManagementAdminCredential credential = {0};
    size_t credential_length = sizeof(credential);
    const esp_err_t credential_result = nvs_get_blob(handle,
                                                     MANAGEMENT_ADMIN_CREDENTIAL_KEY,
                                                     &credential,
                                                     &credential_length);
    bool credential_loaded = false;
    bool legacy_loaded = false;
    if (credential_result == ESP_OK && credential_length == sizeof(credential) &&
        credential.version == MANAGEMENT_PASSWORD_CREDENTIAL_VERSION &&
        credential.iterations >= MANAGEMENT_PASSWORD_MIN_ITERATIONS &&
        credential.iterations <= MANAGEMENT_PASSWORD_MAX_ITERATIONS)
    {
        memcpy(salt, credential.salt, sizeof(salt));
        memcpy(stored_hash, credential.hash, sizeof(stored_hash));
        iterations = credential.iterations;
        credential_loaded = true;
    }
    else if (credential_result == ESP_ERR_NVS_NOT_FOUND)
    {
        size_t salt_length = sizeof(salt);
        size_t hash_length = sizeof(stored_hash);
        const esp_err_t salt_result = nvs_get_blob(handle, MANAGEMENT_ADMIN_SALT_KEY,
                                                   salt, &salt_length);
        const esp_err_t hash_result = nvs_get_blob(handle, MANAGEMENT_ADMIN_HASH_KEY,
                                                   stored_hash, &hash_length);
        legacy_loaded = salt_result == ESP_OK && hash_result == ESP_OK &&
                        salt_length == sizeof(salt) && hash_length == sizeof(stored_hash);
        iterations = MANAGEMENT_PASSWORD_LEGACY_ITERATIONS;
    }
    nvs_close(handle);
    mbedtls_platform_zeroize(&credential, sizeof(credential));
    if ((!credential_loaded && !legacy_loaded) ||
        management_derive_password_hash(password, salt, iterations,
                                        candidate_hash) != ESP_OK)
    {
        mbedtls_platform_zeroize(salt, sizeof(salt));
        mbedtls_platform_zeroize(stored_hash, sizeof(stored_hash));
        mbedtls_platform_zeroize(candidate_hash, sizeof(candidate_hash));
        return false;
    }

    const bool matches = management_constant_time_equal(stored_hash, candidate_hash,
                                                        sizeof(stored_hash));
    if (matches && legacy_loaded && needs_migration != NULL)
    {
        *needs_migration = true;
    }
    mbedtls_platform_zeroize(salt, sizeof(salt));
    mbedtls_platform_zeroize(stored_hash, sizeof(stored_hash));
    mbedtls_platform_zeroize(candidate_hash, sizeof(candidate_hash));
    return matches;
}

static esp_err_t management_nvs_load_blob(const char *key, uint8_t **value,
                                          size_t *value_length)
{
    nvs_handle_t handle = 0;
    esp_err_t result = management_open_nvs(NVS_READONLY, &handle);
    if (result != ESP_OK)
    {
        return result;
    }

    size_t length = 0;
    result = nvs_get_blob(handle, key, NULL, &length);
    if (result != ESP_OK || length == 0)
    {
        nvs_close(handle);
        return result == ESP_OK ? ESP_ERR_NVS_NOT_FOUND : result;
    }

    uint8_t *buffer = calloc(1, length + 1);
    if (buffer == NULL)
    {
        nvs_close(handle);
        return ESP_ERR_NO_MEM;
    }

    result = nvs_get_blob(handle, key, buffer, &length);
    nvs_close(handle);
    if (result != ESP_OK)
    {
        free(buffer);
        return result;
    }

    *value = buffer;
    *value_length = length;
    return ESP_OK;
}

static esp_err_t management_store_certificate_material(const uint8_t *certificate,
                                                       size_t certificate_length,
                                                       const uint8_t *private_key,
                                                       size_t private_key_length)
{
    nvs_handle_t handle = 0;
    esp_err_t result = management_open_nvs(NVS_READWRITE, &handle);
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, MANAGEMENT_CERTIFICATE_KEY, certificate,
                              certificate_length);
    }
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, MANAGEMENT_PRIVATE_KEY_KEY, private_key,
                              private_key_length);
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    if (handle != 0)
    {
        nvs_close(handle);
    }
    return result;
}

static esp_err_t management_generate_certificate_material(void)
{
    uint8_t mac[6];
    ESP_RETURN_ON_ERROR(esp_read_mac(mac, ESP_MAC_WIFI_STA), TAG,
                        "Unable to read the Wi-Fi station MAC address");

    uint8_t *certificate = calloc(1, MANAGEMENT_CERTIFICATE_BUFFER_SIZE);
    uint8_t *private_key = calloc(1, MANAGEMENT_PRIVATE_KEY_BUFFER_SIZE);
    if (certificate == NULL || private_key == NULL)
    {
        free(certificate);
        free(private_key);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t result = ESP_FAIL;
    mbedtls_pk_context key;
    mbedtls_x509write_cert certificate_writer;
    psa_key_id_t psa_key = 0;
    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&certificate_writer);
    int mbedtls_result = 0;
    psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&key_attributes, 256);
    psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_status_t psa_result = psa_crypto_init();
    if (psa_result == PSA_SUCCESS)
    {
        psa_result = psa_generate_key(&key_attributes, &psa_key);
    }
    psa_reset_key_attributes(&key_attributes);
    if (psa_result == PSA_SUCCESS)
    {
        mbedtls_result = mbedtls_pk_wrap_psa(&key, psa_key);
    }
    else
    {
        mbedtls_result = (int)psa_result;
    }
    if (mbedtls_result == 0)
    {
        mbedtls_x509write_crt_set_subject_key(&certificate_writer, &key);
        mbedtls_x509write_crt_set_issuer_key(&certificate_writer, &key);
        mbedtls_x509write_crt_set_version(&certificate_writer,
                                          MBEDTLS_X509_CRT_VERSION_3);
        mbedtls_x509write_crt_set_md_alg(&certificate_writer, MBEDTLS_MD_SHA256);

        char subject[48];
        snprintf(subject, sizeof(subject), "CN=ESP32-NUT-%02X%02X%02X",
                 mac[3], mac[4], mac[5]);
        mbedtls_result = mbedtls_x509write_crt_set_subject_name(&certificate_writer,
                                                                 subject);
        if (mbedtls_result == 0)
        {
            mbedtls_result = mbedtls_x509write_crt_set_issuer_name(&certificate_writer,
                                                                    subject);
        }
    }

    uint8_t serial_bytes[16];
    if (mbedtls_result == 0)
    {
        esp_fill_random(serial_bytes, sizeof(serial_bytes));
        serial_bytes[0] &= 0x7f;
        if (serial_bytes[0] == 0)
        {
            serial_bytes[0] = 1;
        }
        mbedtls_result = mbedtls_x509write_crt_set_serial_raw(&certificate_writer, serial_bytes,
                                                              sizeof(serial_bytes));
    }
    if (mbedtls_result == 0)
    {
        mbedtls_result = mbedtls_x509write_crt_set_validity(&certificate_writer,
                                                            "20260101000000",
                                                            "20500101000000");
    }
    if (mbedtls_result == 0)
    {
        mbedtls_result = mbedtls_x509write_crt_set_basic_constraints(&certificate_writer,
                                                                      0, -1);
    }
    if (mbedtls_result == 0)
    {
        mbedtls_result = mbedtls_x509write_crt_set_key_usage(
            &certificate_writer, MBEDTLS_X509_KU_DIGITAL_SIGNATURE |
                                 MBEDTLS_X509_KU_KEY_ENCIPHERMENT);
    }
    if (mbedtls_result == 0)
    {
        mbedtls_result = mbedtls_x509write_crt_pem(&certificate_writer, certificate,
                                                    MANAGEMENT_CERTIFICATE_BUFFER_SIZE);
    }
    if (mbedtls_result == 0)
    {
        mbedtls_result = mbedtls_pk_write_key_pem(&key, private_key,
                                                  MANAGEMENT_PRIVATE_KEY_BUFFER_SIZE);
    }
    if (mbedtls_result == 0)
    {
        const size_t certificate_length = strlen((char *)certificate) + 1;
        const size_t private_key_length = strlen((char *)private_key) + 1;
        result = management_store_certificate_material(certificate, certificate_length,
                                                        private_key, private_key_length);
        if (result == ESP_OK)
        {
            management_certificate = certificate;
            management_certificate_length = certificate_length;
            management_private_key = private_key;
            management_private_key_length = private_key_length;
            certificate = NULL;
            private_key = NULL;
            ESP_LOGI(TAG, "Generated a device-specific self-signed HTTPS certificate");
        }
    }

    mbedtls_platform_zeroize(serial_bytes, sizeof(serial_bytes));
    mbedtls_x509write_crt_free(&certificate_writer);
    mbedtls_pk_free(&key);
    if (psa_key != 0)
    {
        psa_destroy_key(psa_key);
    }
    free(certificate);
    if (private_key != NULL)
    {
        mbedtls_platform_zeroize(private_key, MANAGEMENT_PRIVATE_KEY_BUFFER_SIZE);
    }
    free(private_key);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to generate HTTPS certificate material (mbedTLS %d, ESP-IDF %s)",
                 mbedtls_result, esp_err_to_name(result));
    }
    return result;
}

static esp_err_t management_load_or_create_certificate(void)
{
    esp_err_t certificate_result = management_nvs_load_blob(MANAGEMENT_CERTIFICATE_KEY,
                                                            &management_certificate,
                                                            &management_certificate_length);
    esp_err_t key_result = management_nvs_load_blob(MANAGEMENT_PRIVATE_KEY_KEY,
                                                    &management_private_key,
                                                    &management_private_key_length);
    if (certificate_result == ESP_OK && key_result == ESP_OK)
    {
        return ESP_OK;
    }

    free(management_certificate);
    management_certificate = NULL;
    management_certificate_length = 0;
    if (management_private_key != NULL)
    {
        mbedtls_platform_zeroize(management_private_key, management_private_key_length);
    }
    free(management_private_key);
    management_private_key = NULL;
    management_private_key_length = 0;
    return management_generate_certificate_material();
}

static bool management_extract_form_value(char *body, const char *expected_name,
                                          char *destination, size_t destination_size)
{
    char *save_pointer = NULL;
    for (char *pair = strtok_r(body, "&", &save_pointer); pair != NULL;
         pair = strtok_r(NULL, "&", &save_pointer))
    {
        char *separator = strchr(pair, '=');
        if (separator == NULL)
        {
            continue;
        }
        *separator = '\0';
        char *encoded_value = separator + 1;
        char decoded_name[40];
        size_t name_length = 0;
        size_t name_index = 0;
        for (; pair[name_index] != '\0' && name_length + 1 < sizeof(decoded_name); name_index++)
        {
            if (pair[name_index] == '%' && pair[name_index + 1] != '\0' && pair[name_index + 2] != '\0')
            {
                char hexadecimal[3] = {pair[name_index + 1], pair[name_index + 2], '\0'};
                decoded_name[name_length++] = (char)strtol(hexadecimal, NULL, 16);
                name_index += 2;
            }
            else
            {
                decoded_name[name_length++] = pair[name_index] == '+' ? ' ' : pair[name_index];
            }
        }
        decoded_name[name_length] = '\0';
        if (pair[name_index] != '\0' || strcmp(decoded_name, expected_name) != 0)
        {
            continue;
        }

        size_t value_length = 0;
        size_t value_index = 0;
        for (; encoded_value[value_index] != '\0' && value_length + 1 < destination_size; value_index++)
        {
            if (encoded_value[value_index] == '%' && encoded_value[value_index + 1] != '\0' &&
                encoded_value[value_index + 2] != '\0')
            {
                char hexadecimal[3] = {encoded_value[value_index + 1], encoded_value[value_index + 2], '\0'};
                destination[value_length++] = (char)strtol(hexadecimal, NULL, 16);
                value_index += 2;
            }
            else
            {
                destination[value_length++] = encoded_value[value_index] == '+' ? ' ' : encoded_value[value_index];
            }
        }
        destination[value_length] = '\0';
        return encoded_value[value_index] == '\0';
    }
    return false;
}

static esp_err_t management_read_form_body(httpd_req_t *request, char *body,
                                           size_t body_size)
{
    if (request->content_len <= 0 || request->content_len > MANAGEMENT_FORM_BODY_LIMIT ||
        (size_t)request->content_len >= body_size)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    int received_total = 0;
    while (received_total < request->content_len)
    {
        const int received = httpd_req_recv(request, body + received_total,
                                            request->content_len - received_total);
        if (received <= 0)
        {
            return received == HTTPD_SOCK_ERR_TIMEOUT ? ESP_ERR_TIMEOUT : ESP_FAIL;
        }
        received_total += received;
    }
    body[received_total] = '\0';
    return ESP_OK;
}

static bool management_form_value(const char *body, const char *name,
                                  char *destination, size_t destination_size)
{
    char body_copy[MANAGEMENT_FORM_BODY_LIMIT + 1];
    snprintf(body_copy, sizeof(body_copy), "%s", body);
    const bool found = management_extract_form_value(body_copy, name, destination,
                                                      destination_size);
    mbedtls_platform_zeroize(body_copy, sizeof(body_copy));
    return found;
}

static bool management_cookie_value(httpd_req_t *request, const char *name,
                                    char *destination, size_t destination_size)
{
    const size_t cookie_length = httpd_req_get_hdr_value_len(request, "Cookie");
    if (cookie_length == 0 || cookie_length > 256 || destination_size == 0)
    {
        return false;
    }

    char cookie_header[257];
    if (httpd_req_get_hdr_value_str(request, "Cookie", cookie_header,
                                    sizeof(cookie_header)) != ESP_OK)
    {
        return false;
    }

    const size_t name_length = strlen(name);
    char *entry = cookie_header;
    while (*entry != '\0')
    {
        while (*entry == ' ' || *entry == ';')
        {
            entry++;
        }
        char *separator = strchr(entry, '=');
        if (separator == NULL)
        {
            break;
        }
        char *end = strchr(separator + 1, ';');
        if (end == NULL)
        {
            end = entry + strlen(entry);
        }
        const size_t entry_name_length = (size_t)(separator - entry);
        const size_t value_length = (size_t)(end - separator - 1);
        if (entry_name_length == name_length && strncmp(entry, name, name_length) == 0 &&
            value_length + 1 <= destination_size)
        {
            memcpy(destination, separator + 1, value_length);
            destination[value_length] = '\0';
            return true;
        }
        entry = end;
    }
    return false;
}

static bool management_is_hex_token(const char *token)
{
    if (token == NULL || strlen(token) != MANAGEMENT_SESSION_HEX_LENGTH)
    {
        return false;
    }
    for (size_t index = 0; index < MANAGEMENT_SESSION_HEX_LENGTH; index++)
    {
        const char character = token[index];
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f')))
        {
            return false;
        }
    }
    return true;
}

static void management_start_session(void)
{
    uint8_t cookie[MANAGEMENT_SESSION_BYTES];
    uint8_t csrf[MANAGEMENT_SESSION_BYTES];
    esp_fill_random(cookie, sizeof(cookie));
    esp_fill_random(csrf, sizeof(csrf));

    taskENTER_CRITICAL(&management_session_lock);
    management_bytes_to_hex(cookie, sizeof(cookie), management_session.cookie,
                            sizeof(management_session.cookie));
    management_bytes_to_hex(csrf, sizeof(csrf), management_session.csrf,
                            sizeof(management_session.csrf));
    management_session.last_activity_us = esp_timer_get_time();
    management_session.active = true;
    taskEXIT_CRITICAL(&management_session_lock);
    mbedtls_platform_zeroize(cookie, sizeof(cookie));
    mbedtls_platform_zeroize(csrf, sizeof(csrf));
}

static void management_set_session_cookie(httpd_req_t *request, char *session_header,
                                          size_t session_header_size)
{
    taskENTER_CRITICAL(&management_session_lock);
    snprintf(session_header, session_header_size,
             "ESP32NUT_SESSION=%s; Path=/; Secure; HttpOnly; SameSite=Strict",
             management_session.cookie);
    taskEXIT_CRITICAL(&management_session_lock);
    httpd_resp_set_hdr(request, "Set-Cookie", session_header);
}

static void management_start_setup_session(httpd_req_t *request, char *csrf,
                                           size_t csrf_size, char *setup_header,
                                           size_t setup_header_size)
{
    char cookie[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    if (!management_cookie_value(request, "ESP32NUT_SETUP", cookie, sizeof(cookie)) ||
        !management_is_hex_token(cookie))
    {
        uint8_t cookie_bytes[MANAGEMENT_SESSION_BYTES];
        esp_fill_random(cookie_bytes, sizeof(cookie_bytes));
        management_bytes_to_hex(cookie_bytes, sizeof(cookie_bytes), cookie, sizeof(cookie));
        mbedtls_platform_zeroize(cookie_bytes, sizeof(cookie_bytes));
    }
    snprintf(csrf, csrf_size, "%s", cookie);
    snprintf(setup_header, setup_header_size,
             "ESP32NUT_SETUP=%s; Path=/; Max-Age=300; Secure; HttpOnly; SameSite=Strict",
             cookie);
    httpd_resp_set_hdr(request, "Set-Cookie", setup_header);
    mbedtls_platform_zeroize(cookie, sizeof(cookie));
}

static bool management_setup_csrf_is_valid(httpd_req_t *request, const char *csrf)
{
    char cookie[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    if (!management_is_hex_token(csrf) ||
        !management_cookie_value(request, "ESP32NUT_SETUP", cookie, sizeof(cookie)) ||
        !management_is_hex_token(cookie))
    {
        return false;
    }

    const bool valid = management_constant_time_equal((const uint8_t *)cookie,
                                                      (const uint8_t *)csrf,
                                                      MANAGEMENT_SESSION_HEX_LENGTH);
    mbedtls_platform_zeroize(cookie, sizeof(cookie));
    return valid;
}

static int management_login_retry_after_seconds(int64_t now)
{
    int retry_after = 0;
    taskENTER_CRITICAL(&management_login_lock);
    if (now < management_login_cooldown_until_us)
    {
        retry_after = (int)((management_login_cooldown_until_us - now + 999999LL) /
                            1000000LL);
    }
    taskEXIT_CRITICAL(&management_login_lock);
    return retry_after;
}

static bool management_record_login_failure(int64_t now)
{
    bool cooldown_started = false;
    taskENTER_CRITICAL(&management_login_lock);
    management_login_failures++;
    if (management_login_failures >= MANAGEMENT_LOGIN_MAX_FAILURES)
    {
        management_login_failures = 0;
        management_login_cooldown_until_us = now + MANAGEMENT_LOGIN_COOLDOWN_US;
        cooldown_started = true;
    }
    taskEXIT_CRITICAL(&management_login_lock);
    return cooldown_started;
}

static void management_record_login_success(void)
{
    taskENTER_CRITICAL(&management_login_lock);
    management_login_failures = 0;
    management_login_cooldown_until_us = 0;
    taskEXIT_CRITICAL(&management_login_lock);
}

static void management_clear_session(void)
{
    taskENTER_CRITICAL(&management_session_lock);
    mbedtls_platform_zeroize(&management_session, sizeof(management_session));
    taskEXIT_CRITICAL(&management_session_lock);
}

static bool management_cookie_is_authorized(httpd_req_t *request)
{
    char value[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    if (!management_cookie_value(request, "ESP32NUT_SESSION", value, sizeof(value)) ||
        strlen(value) != MANAGEMENT_SESSION_HEX_LENGTH)
    {
        return false;
    }

    bool authorized = false;
    taskENTER_CRITICAL(&management_session_lock);
    const int64_t now = esp_timer_get_time();
    if (management_session.active &&
        now - management_session.last_activity_us <= MANAGEMENT_SESSION_IDLE_US &&
        management_constant_time_equal((const uint8_t *)value,
                                       (const uint8_t *)management_session.cookie,
                                       MANAGEMENT_SESSION_HEX_LENGTH))
    {
        management_session.last_activity_us = now;
        authorized = true;
    }
    else if (management_session.active && now - management_session.last_activity_us > MANAGEMENT_SESSION_IDLE_US)
    {
        mbedtls_platform_zeroize(&management_session, sizeof(management_session));
    }
    taskEXIT_CRITICAL(&management_session_lock);
    mbedtls_platform_zeroize(value, sizeof(value));
    return authorized;
}

static bool management_csrf_is_valid(httpd_req_t *request)
{
    if (!management_cookie_is_authorized(request))
    {
        return false;
    }

    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1] = {0};
    if (httpd_req_get_hdr_value_str(request, "X-ESP32-NUT-CSRF", csrf,
                                    sizeof(csrf)) != ESP_OK ||
        strlen(csrf) != MANAGEMENT_SESSION_HEX_LENGTH)
    {
        return false;
    }

    bool matches;
    taskENTER_CRITICAL(&management_session_lock);
    matches = management_constant_time_equal((const uint8_t *)csrf,
                                             (const uint8_t *)management_session.csrf,
                                             MANAGEMENT_SESSION_HEX_LENGTH);
    taskEXIT_CRITICAL(&management_session_lock);
    return matches;
}

static bool management_require_session(httpd_req_t *request)
{
    if (management_cookie_is_authorized(request))
    {
        return true;
    }
    management_send_json(request, "401 Unauthorized",
                         "{\"error\":\"ADMIN authentication is required.\"}");
    return false;
}

static bool management_bearer_is_authorized(httpd_req_t *request,
                                             uint32_t required_scope)
{
    static const char prefix[] = "Bearer ";
    const size_t expected_length = sizeof(prefix) - 1U + API_TOKEN_VALUE_LENGTH;
    const size_t header_length =
        httpd_req_get_hdr_value_len(request, "Authorization");
    if (header_length != expected_length)
    {
        return false;
    }

    char authorization[sizeof(prefix) - 1U + API_TOKEN_VALUE_LENGTH + 1U];
    if (httpd_req_get_hdr_value_str(request, "Authorization", authorization,
                                    sizeof(authorization)) != ESP_OK)
    {
        mbedtls_platform_zeroize(authorization, sizeof(authorization));
        return false;
    }
    const bool authorized =
        strncmp(authorization, prefix, sizeof(prefix) - 1U) == 0 &&
        api_tokens_authorize(authorization + sizeof(prefix) - 1U,
                             required_scope);
    mbedtls_platform_zeroize(authorization, sizeof(authorization));
    return authorized;
}

static esp_err_t management_send_bearer_unauthorized(httpd_req_t *request)
{
    httpd_resp_set_hdr(
        request, "WWW-Authenticate",
        "Bearer realm=\"ESP32-NUT Agent OTA\", scope=\"ota.install\"");
    return management_send_json(
        request, "401 Unauthorized",
        "{\"error\":\"A valid API token with ota.install scope is required.\"}");
}

static const char management_setup_page_template[] =
    "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>ESP32-NUT setup</title><style>body{font:17px -apple-system,BlinkMacSystemFont,sans-serif;margin:2rem;max-width:42rem;color:#17212b}input,button{font:inherit;padding:.75rem;width:100%%;box-sizing:border-box;margin:.35rem 0 1rem}button{background:#267747;color:white;border:0;border-radius:.4rem;font-weight:600}.hint{color:#52606d}.check{display:flex;gap:.5rem;align-items:center}.check input{width:auto;margin:0}</style>"
    "<h1>ESP32-NUT administrator setup</h1><p>Choose the unique ADMIN password for this device. It is never stored in plaintext.</p>"
    "<form method=post action=/setup><input name=csrf type=hidden value='%s'><label>ADMIN password<input id=password name=password type=password required minlength=12 maxlength=128 autocomplete=new-password></label>"
    "<label>Confirm ADMIN password<input id=confirmPassword name=confirm type=password required minlength=12 maxlength=128 autocomplete=new-password></label>"
    "<label class=check><input id=show type=checkbox> Show password</label><button type=submit>Save administrator password</button></form>"
    "<p class=hint>Use at least 12 characters. A physical recovery action can return the device to this setup screen.</p>"
    "<script>const passwordInput=document.getElementById('password'),confirmationInput=document.getElementById('confirmPassword');document.getElementById('show').onchange=e=>{passwordInput.type=confirmationInput.type=e.target.checked?'text':'password'}</script>";

static const char management_login_page[] =
    "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>ESP32-NUT sign in</title><style>body{font:17px -apple-system,BlinkMacSystemFont,sans-serif;margin:2rem;max-width:32rem;color:#17212b}input,button{font:inherit;padding:.75rem;width:100%;box-sizing:border-box;margin:.35rem 0 1rem}button{background:#267747;color:white;border:0;border-radius:.4rem;font-weight:600}.check{display:flex;gap:.5rem;align-items:center}.check input{width:auto;margin:0}</style>"
    "<h1>ESP32-NUT</h1><p>Sign in as ADMIN.</p><form id=loginForm method=post action=/login><label>ADMIN password<input id=password name=password type=password required autocomplete=current-password></label>"
    "<label class=check><input id=show type=checkbox> Show password</label><button id=signIn type=submit>Sign in</button></form><p id=loginResult role=status></p>"
    "<script>const loginForm=document.getElementById('loginForm'),password=document.getElementById('password'),show=document.getElementById('show'),signIn=document.getElementById('signIn'),loginResult=document.getElementById('loginResult');show.onchange=()=>password.type=show.checked?'text':'password';loginForm.onsubmit=e=>{e.preventDefault();signIn.disabled=true;loginResult.textContent='Verifying password…';requestAnimationFrame(()=>requestAnimationFrame(()=>loginForm.submit()))}</script>";

static esp_err_t management_send_login_throttled(httpd_req_t *request, int retry_after);

static esp_err_t management_root_handler(httpd_req_t *request)
{
    if (!management_admin_password_is_configured())
    {
        char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
        char setup_header[192];
        management_start_setup_session(request, csrf, sizeof(csrf), setup_header,
                                       sizeof(setup_header));
        char page[1800];
        snprintf(page, sizeof(page), management_setup_page_template, csrf);
        mbedtls_platform_zeroize(csrf, sizeof(csrf));
        return management_send_html(request, page);
    }
    if (!management_cookie_is_authorized(request))
    {
        const int retry_after = management_login_retry_after_seconds(esp_timer_get_time());
        if (retry_after > 0)
        {
            return management_send_login_throttled(request, retry_after);
        }
        return management_send_html(request, management_login_page);
    }

    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    taskENTER_CRITICAL(&management_session_lock);
    snprintf(csrf, sizeof(csrf), "%s", management_session.csrf);
    taskEXIT_CRITICAL(&management_session_lock);

    char *page = calloc(1, MANAGEMENT_ADMIN_PAGE_SIZE);
    if (page == NULL)
    {
        mbedtls_platform_zeroize(csrf, sizeof(csrf));
        return management_send_html_status(
            request, "500 Internal Server Error",
            "<h1>ESP32-NUT administration</h1><p>Unable to prepare the administration page.</p>");
    }
    const int page_length = snprintf(page, MANAGEMENT_ADMIN_PAGE_SIZE,
             "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
             "<title>ESP32-NUT administration</title><style>body{font:17px -apple-system,BlinkMacSystemFont,sans-serif;margin:2rem auto;max-width:60rem;padding:0 1rem;color:#17212b}pre{background:#f0f3f5;padding:1rem;overflow:auto}input,button,select{font:inherit;padding:.7rem;width:100%%;box-sizing:border-box;margin:.35rem 0 1rem}button{background:#267747;color:white;border:0;border-radius:.4rem}.secondary{background:#52606d}.danger{background:#a12622}.check{display:flex;gap:.5rem;align-items:center;margin-bottom:1rem}.check input{width:auto;margin:0}.result{min-height:1.5rem}.hint{color:#52606d}.token-once{border:2px solid #b7791f;background:#fffaf0;padding:1rem;margin:1rem 0}.token-once code{display:block;overflow-wrap:anywhere;margin:.75rem 0;font-size:.95rem}.token-row{border-top:1px solid #cbd5e1;padding:.8rem 0}.token-row button{margin:.6rem 0 0}.actions{display:flex;gap:.75rem}.dashboard-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(13rem,1fr));gap:.75rem}.card{border:1px solid #cbd5e1;border-radius:.5rem;padding:1rem;background:#fff}.card h3{margin:.1rem 0 .75rem;font-size:1.05rem}.metric{margin:.45rem 0}.metric strong{display:block;font-size:.95rem}.metric span{display:block;margin-top:.15rem;overflow-wrap:anywhere}.tabs{display:flex;flex-wrap:wrap;gap:.35rem;margin:1.25rem 0;border-bottom:1px solid #cbd5e1;padding-bottom:.75rem}.tab{width:auto;margin:0;padding:.55rem .8rem;background:#52606d;white-space:nowrap}.tab[aria-selected=true]{background:#267747;font-weight:600}.panel[hidden]{display:none}.panel{padding:.25rem 0 1rem}.network-list{display:grid;gap:.5rem;margin:1rem 0}.network-list[hidden]{display:none}.network-option{display:flex;align-items:center;justify-content:space-between;gap:1rem;text-align:left;background:#f0f3f5;color:#17212b;border:1px solid #cbd5e1}.network-option:hover,.network-option[aria-pressed=true]{background:#dbeafe;border-color:#267747}.network-name{font-weight:600;overflow-wrap:anywhere}.network-details{color:#52606d;font-size:.9rem;white-space:nowrap}dialog{max-width:32rem;border:0;border-radius:.5rem;padding:1.25rem;box-shadow:0 1rem 3rem #0006}dialog::backdrop{background:#0008}</style>"
             "<style>.button{display:inline-block;font:inherit;padding:.7rem;box-sizing:border-box;border-radius:.4rem;text-align:center;text-decoration:none;color:white}.actions>*{flex:1;margin:0}</style>"
             "<h1>ESP32-NUT administration</h1><p>HTTPS is active with this device's self-signed certificate."
             " The administration API is LAN-only.</p><nav class=tabs aria-label='Administration sections'>"
             "<button class=tab type=button data-panel=dashboard aria-selected=true>Dashboard</button>"
             "<button class=tab type=button data-panel=status aria-selected=false>Device Status</button>"
             "<button class=tab type=button data-panel=time aria-selected=false>Date and Time</button>"
             "<button class=tab type=button data-panel=wifi aria-selected=false>Wi-Fi Configuration</button>"
             "<button class=tab type=button data-panel=password aria-selected=false>ADMIN Password</button>"
             "<button class=tab type=button data-panel=tokens aria-selected=false>API Tokens</button>"
             "<button class=tab type=button data-panel=ota aria-selected=false>Update Firmware</button></nav><main>"
             "<section id=panel-dashboard class=panel><h2>Dashboard</h2><section class=dashboard-grid>"
             "<article class=card><h3>Device</h3><p class=metric><strong>Firmware</strong><span id=dashboardFirmware>Loading…</span></p>"
             "<p class=metric><strong>Uptime</strong><span id=dashboardUptime>Loading…</span></p>"
             "<p class=metric><strong>Last update</strong><span id=dashboardUpdate>Loading…</span></p></article>"
             "<article class=card><h3>Wi-Fi</h3><p class=metric><strong>Connection</strong><span id=dashboardWifi>Loading…</span></p>"
             "<p class=metric><strong>Signal</strong><span id=dashboardSignal>Loading…</span></p></article>"
             "<article class=card><h3>NUT service</h3><p class=metric><strong>Health</strong><span id=dashboardNut>Loading…</span></p>"
             "<p class=metric><strong>UPS status</strong><span id=dashboardUpsStatus>Loading…</span></p></article>"
             "<article class=card><h3>UPS identity</h3><p class=metric><strong>Model</strong><span id=dashboardUps>Loading…</span></p>"
             "<p class=metric><strong>Serial number</strong><span id=dashboardSerial>Loading…</span></p></article>"
             "<article class=card><h3>Battery and load</h3><p class=metric><strong>Charge</strong><span id=dashboardBattery>Loading…</span></p>"
             "<p class=metric><strong>Runtime</strong><span id=dashboardRuntime>Loading…</span></p>"
             "<p class=metric><strong>Load</strong><span id=dashboardLoad>Loading…</span></p></article>"
             "<article class=card><h3>Power</h3><p class=metric><strong>Battery voltage</strong><span id=dashboardBatteryVoltage>Loading…</span></p>"
             "<p class=metric><strong>Input voltage</strong><span id=dashboardInputVoltage>Loading…</span></p>"
             "<p class=metric><strong>Output voltage</strong><span id=dashboardOutputVoltage>Loading…</span></p></article></section></section>"
             "<section id=panel-status class=panel hidden><h2>Device Status</h2><details><summary>Raw status JSON</summary><pre id=status>Loading…</pre></details></section>"
             "<section id=panel-time class=panel hidden><h2>Date and Time</h2><p id=timeSummary>Loading time status…</p>"
             "<form id=timeConfigForm><label class=check><input id=ntpEnabled type=checkbox> Synchronize automatically with NTP</label>"
             "<label>NTP server<input id=ntpServer name=ntp_server required maxlength=63 autocomplete=off></label>"
             "<label>Time zone<select id=timeZone name=timezone required>"
             "<option value='UTC'>UTC</option><option value='America/Los_Angeles'>America/Los_Angeles</option>"
             "<option value='America/Denver'>America/Denver</option><option value='America/Phoenix'>America/Phoenix</option>"
             "<option value='America/Chicago'>America/Chicago</option><option value='America/New_York'>America/New_York</option>"
             "<option value='America/Anchorage'>America/Anchorage</option><option value='Pacific/Honolulu'>Pacific/Honolulu</option>"
             "</select></label><button type=submit>Save time settings</button></form>"
             "<button id=syncNow class=secondary type=button>Synchronize now</button>"
             "<form id=manualTimeForm><label>Manual date and time in the selected time zone<input id=manualDateTime name=local_datetime type=datetime-local min='2024-01-01T00:00' max='2099-12-31T23:59' required></label>"
             "<button class=secondary type=submit>Set date and time manually</button></form>"
             "<p class=hint>Manual time remains available while NTP retries and is replaced after a successful synchronization.</p><p id=timeResult class=result role=status></p></section>"
             "<section id=panel-wifi class=panel hidden><h2>Wi-Fi Configuration</h2><p id=wifiCurrent>Loading current Wi-Fi status…</p>"
             "<p class=hint>ESP32-NUT scans supported 2.4 GHz networks. Scanning may briefly affect the station connection.</p>"
             "<button id=wifiScanButton class=secondary type=button>Scan for networks</button><p id=wifiScanResult class=result role=status></p>"
             "<form id=wifiForm><label>Wi-Fi network<input id=wifiSsid name=ssid required maxlength=32 autocomplete=off></label><div id=wifiNetworkList class=network-list role=list hidden></div>"
             "<label>Wi-Fi password<input id=wifiPassword name=password type=password maxlength=63 autocomplete=new-password></label>"
             "<label class=check><input id=wifiShowPassword type=checkbox> Show password</label>"
             "<p class=hint>The stored password is never displayed. Enter a password only when changing networks; leave it blank for an open network.</p>"
             "<button id=wifiConfigureButton type=submit>Save and reconnect</button></form><p id=wifiResult class=result role=status></p></section>"
             "<section id=panel-password class=panel hidden><h2>ADMIN Password</h2><form id=passwordForm><label>Current password<input id=currentPassword name=current type=password required autocomplete=current-password></label>"
             "<label>New password<input id=newPassword name=password type=password required minlength=12 maxlength=128 autocomplete=new-password></label>"
             "<label>Confirm new password<input id=confirmPassword name=confirm type=password required minlength=12 maxlength=128 autocomplete=new-password></label>"
             "<label class=check><input id=showPasswords type=checkbox> Show passwords</label><button type=submit>Change password</button></form><p id=passwordResult class=result role=status></p></section>"
             "<section id=panel-tokens class=panel hidden><h2>API Tokens</h2><p>Create up to four named, non-expiring tokens. In this release every token is limited to Agent-driven firmware installation.</p>"
             "<form id=tokenForm><label>Token name<input id=tokenName name=name required minlength=1 maxlength=32 pattern='[-A-Za-z0-9._ ]+' autocomplete=off></label><button type=submit>Create API token</button></form>"
             "<section id=tokenOnce class=token-once hidden><strong>Copy this token now. It will never be shown again.</strong><code id=tokenValue></code><span id=tokenMetadata></span></section>"
             "<div id=tokenList>Loading API tokens…</div><p id=tokenResult class=result role=status></p>"
             "<dialog id=deleteTokenDialog><h3>Delete API token</h3><p>Delete <strong id=deleteTokenName></strong>? Requests using it will be rejected immediately.</p>"
             "<label class=check><input id=deleteTokenAck type=checkbox> I acknowledge that this token will be permanently revoked.</label>"
             "<div class=actions><button id=deleteTokenCancel class=secondary type=button>Cancel</button><button id=deleteTokenConfirm class=danger type=button disabled>Delete token</button></div></dialog></section>"
             "<section id=panel-ota class=panel hidden><h2>Update Firmware</h2><p>Select a local ESP32-NUT application image. Check it before choosing whether to install it into the inactive OTA slot.</p>"
             "<p class=hint>Download release images and their checksums in your browser from the <a class='button secondary' href='https://github.com/BillyFKidney/esp32-nut-server/releases/latest' target=_blank rel='noopener noreferrer'>ESP32-NUT release page</a>. The device never fetches firmware from a remote source.</p>"
             "<form id=otaForm><label>Firmware .bin file<input id=otaFile type=file accept='.bin,application/octet-stream' required></label><div class=actions><button id=otaCheckButton class=secondary type=button>Check firmware</button><button id=otaButton type=submit>Install firmware</button></div></form><p id=otaResult class=result role=status></p></section></main>"
             "<p class=hint>All management actions remain protected by the ADMIN browser session.</p>"
             "<button onclick=logout()>Sign out</button><script>"
             "const csrf='%s',status=document.getElementById('status'),timeSummary=document.getElementById('timeSummary'),timeConfigForm=document.getElementById('timeConfigForm'),ntpEnabled=document.getElementById('ntpEnabled'),ntpServer=document.getElementById('ntpServer'),timeZone=document.getElementById('timeZone'),syncNow=document.getElementById('syncNow'),manualTimeForm=document.getElementById('manualTimeForm'),manualDateTime=document.getElementById('manualDateTime'),timeResult=document.getElementById('timeResult'),wifiCurrent=document.getElementById('wifiCurrent'),wifiScanButton=document.getElementById('wifiScanButton'),wifiScanResult=document.getElementById('wifiScanResult'),wifiForm=document.getElementById('wifiForm'),wifiSsid=document.getElementById('wifiSsid'),wifiNetworkList=document.getElementById('wifiNetworkList'),wifiPassword=document.getElementById('wifiPassword'),wifiShowPassword=document.getElementById('wifiShowPassword'),wifiConfigureButton=document.getElementById('wifiConfigureButton'),wifiResult=document.getElementById('wifiResult'),currentPassword=document.getElementById('currentPassword'),newPassword=document.getElementById('newPassword'),confirmPassword=document.getElementById('confirmPassword'),passwordForm=document.getElementById('passwordForm'),passwordResult=document.getElementById('passwordResult'),tokenForm=document.getElementById('tokenForm'),tokenOnce=document.getElementById('tokenOnce'),tokenValue=document.getElementById('tokenValue'),tokenMetadata=document.getElementById('tokenMetadata'),tokenList=document.getElementById('tokenList'),tokenResult=document.getElementById('tokenResult'),deleteTokenDialog=document.getElementById('deleteTokenDialog'),deleteTokenName=document.getElementById('deleteTokenName'),deleteTokenAck=document.getElementById('deleteTokenAck'),deleteTokenConfirm=document.getElementById('deleteTokenConfirm'),otaForm=document.getElementById('otaForm'),otaFile=document.getElementById('otaFile'),otaButton=document.getElementById('otaButton'),otaResult=document.getElementById('otaResult');let pendingTokenId='';"
             "const otaCheckButton=document.getElementById('otaCheckButton');"
             "const tabs=document.querySelectorAll('.tab'),panels={dashboard:document.getElementById('panel-dashboard'),status:document.getElementById('panel-status'),time:document.getElementById('panel-time'),wifi:document.getElementById('panel-wifi'),password:document.getElementById('panel-password'),tokens:document.getElementById('panel-tokens'),ota:document.getElementById('panel-ota')};"
             "const dashboardFirmware=document.getElementById('dashboardFirmware'),dashboardUptime=document.getElementById('dashboardUptime'),dashboardUpdate=document.getElementById('dashboardUpdate'),dashboardWifi=document.getElementById('dashboardWifi'),dashboardSignal=document.getElementById('dashboardSignal'),dashboardNut=document.getElementById('dashboardNut'),dashboardUpsStatus=document.getElementById('dashboardUpsStatus'),dashboardUps=document.getElementById('dashboardUps'),dashboardSerial=document.getElementById('dashboardSerial'),dashboardBattery=document.getElementById('dashboardBattery'),dashboardRuntime=document.getElementById('dashboardRuntime'),dashboardLoad=document.getElementById('dashboardLoad'),dashboardBatteryVoltage=document.getElementById('dashboardBatteryVoltage'),dashboardInputVoltage=document.getElementById('dashboardInputVoltage'),dashboardOutputVoltage=document.getElementById('dashboardOutputVoltage');"
             "function displayValue(value){return value===undefined||value===null||value===''?'Not available':String(value)}function formatUptime(seconds){if(typeof seconds!=='number')return displayValue(seconds);const days=Math.floor(seconds/86400),hours=Math.floor(seconds%%86400/3600),minutes=Math.floor(seconds%%3600/60),remaining=Math.floor(seconds%%60);return (days?days+'d ':'')+(hours?hours+'h ':'')+(minutes?minutes+'m ':'')+remaining+'s'}"
             "function selectPanel(name){for(const tab of tabs)tab.setAttribute('aria-selected',tab.dataset.panel===name?'true':'false');for(const panelName in panels)panels[panelName].hidden=panelName!==name}"
             "tabs.forEach(tab=>tab.onclick=()=>selectPanel(tab.dataset.panel));"
             "function renderDashboard(x){const wifi=x.wifi||{},nut=x.nut||{},ups=x.ups||{},update=x.update||{};dashboardFirmware.textContent=displayValue(x.firmware);dashboardUptime.textContent=formatUptime(x.uptime_seconds);dashboardUpdate.textContent=displayValue(update.last_result);dashboardWifi.textContent=displayValue(wifi.ssid)+' — '+displayValue(wifi.ip)+' — '+(wifi.connected?'connected':'not connected');dashboardSignal.textContent=displayValue(wifi.rssi_dbm)+' dBm';dashboardNut.textContent=displayValue(nut.health)+' — TCP '+displayValue(nut.port)+(nut.data_stale?' — data stale':'');dashboardUpsStatus.textContent=displayValue(ups.status);dashboardUps.textContent=displayValue(nut.ups_name)+' — '+displayValue(ups.manufacturer)+' '+displayValue(ups.model);dashboardSerial.textContent=displayValue(ups.serial);dashboardBattery.textContent=displayValue(ups.battery_charge)+' %%';dashboardRuntime.textContent=displayValue(ups.battery_runtime)+' s';dashboardLoad.textContent=displayValue(ups.load)+' %%';dashboardBatteryVoltage.textContent=displayValue(ups.battery_voltage)+' V';dashboardInputVoltage.textContent=displayValue(ups.input_voltage)+' V';dashboardOutputVoltage.textContent=displayValue(ups.output_voltage)+' V'}"
             "function renderWifi(x){const wifi=x.wifi||{};wifiCurrent.textContent='Current network: '+displayValue(wifi.ssid)+' — '+displayValue(wifi.ip)+' — '+(wifi.connected?'connected':'not connected')+' — '+displayValue(wifi.rssi_dbm)+' dBm';if(!wifiSsid.value&&wifi.ssid)wifiSsid.value=wifi.ssid}"
             "async function loadStatus(){try{const r=await fetch('/api/v1/status',{cache:'no-store'}),x=await r.json();status.textContent=JSON.stringify(x,null,2);renderDashboard(x);renderWifi(x);if(x.time){ntpEnabled.checked=x.time.ntp_enabled;ntpServer.value=x.time.ntp_server;timeZone.value=x.time.timezone;syncNow.disabled=!x.time.ntp_enabled;if(x.time.available){timeSummary.textContent=x.time.local+' ('+x.time.timezone+'), UTC '+x.time.utc+', source '+x.time.source+(x.time.synchronization_pending?' — synchronization pending':'');manualDateTime.value=x.time.local.slice(0,16)}else{timeSummary.textContent=x.time.synchronization_pending?'Time is not set; waiting for NTP.':'Time is not set.'}}}catch(error){status.textContent='Unable to load device status.';wifiCurrent.textContent='Unable to load current Wi-Fi status.'}}"
             "wifiShowPassword.onchange=()=>wifiPassword.type=wifiShowPassword.checked?'text':'password';"
             "wifiScanButton.onclick=async()=>{wifiScanButton.disabled=true;wifiScanResult.textContent='Scanning supported 2.4 GHz networks…';try{const r=await fetch('/api/v1/admin/wifi/scan',{cache:'no-store'}),x=await r.json();if(!r.ok)throw new Error(x.error||'Wi-Fi scan failed.');const networks=x.networks||[];wifiNetworkList.replaceChildren();wifiNetworkList.hidden=networks.length===0;for(const network of networks){const option=document.createElement('button'),name=document.createElement('span'),details=document.createElement('span');option.type='button';option.className='network-option';option.setAttribute('aria-pressed',network.ssid===wifiSsid.value?'true':'false');name.className='network-name';name.textContent=network.ssid;details.className='network-details';details.textContent=network.rssi_dbm+' dBm — '+network.security;option.append(name,details);option.onclick=()=>{wifiSsid.value=network.ssid;for(const other of wifiNetworkList.querySelectorAll('.network-option'))other.setAttribute('aria-pressed','false');option.setAttribute('aria-pressed','true');wifiNetworkList.hidden=true;wifiPassword.focus()};wifiNetworkList.append(option)}wifiScanResult.textContent=networks.length+' network(s) found.'}catch(error){wifiNetworkList.replaceChildren();wifiNetworkList.hidden=true;wifiScanResult.textContent=error.message||'Unable to scan Wi-Fi networks.'}finally{wifiScanButton.disabled=false}};"
             "wifiForm.onsubmit=async e=>{e.preventDefault();if(!wifiSsid.value.trim())return;if(!window.confirm('Save these Wi-Fi credentials and restart ESP32-NUT to test the connection?'))return;wifiConfigureButton.disabled=true;wifiResult.textContent='Staging Wi-Fi credentials…';const body=new URLSearchParams({ssid:wifiSsid.value,password:wifiPassword.value,acknowledge:'true'});try{const r=await fetch('/api/v1/admin/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-ESP32-NUT-CSRF':csrf},body}),x=await r.json();wifiResult.textContent=x.message||x.error||'Wi-Fi configuration failed.';if(r.ok)setTimeout(reconnect,5000);else wifiConfigureButton.disabled=false}catch(error){wifiResult.textContent='Connection closed. The device may be restarting…';setTimeout(reconnect,3000)}};"
             "async function submitTime(body){timeResult.textContent='Applying time settings…';try{const r=await fetch('/api/v1/admin/time',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-ESP32-NUT-CSRF':csrf},body}),x=await r.json();timeResult.textContent=x.message||x.error||'Time operation failed.';if(r.ok)setTimeout(loadStatus,500)}catch(error){timeResult.textContent='Unable to reach the time API.'}}"
             "timeConfigForm.onsubmit=e=>{e.preventDefault();const body=new URLSearchParams(new FormData(timeConfigForm));body.set('action','configure');body.set('ntp_enabled',ntpEnabled.checked?'true':'false');submitTime(body)};"
             "manualTimeForm.onsubmit=e=>{e.preventDefault();const body=new URLSearchParams(new FormData(manualTimeForm));body.set('action','manual');submitTime(body)};"
             "syncNow.onclick=()=>submitTime(new URLSearchParams({action:'sync'}));"
             "document.getElementById('showPasswords').onchange=e=>{currentPassword.type=newPassword.type=confirmPassword.type=e.target.checked?'text':'password'};"
             "passwordForm.onsubmit=async e=>{e.preventDefault();passwordResult.textContent='Changing password…';const body=new URLSearchParams(new FormData(passwordForm));const r=await fetch('/api/v1/admin/password',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-ESP32-NUT-CSRF':csrf},body});const x=await r.json();passwordResult.textContent=x.message||x.error||'Password change failed.';if(r.ok){passwordForm.reset();setTimeout(()=>location='/',3000)}};"
             "async function loadTokens(){tokenList.textContent='Loading API tokens…';try{const r=await fetch('/api/v1/admin/tokens',{cache:'no-store'}),x=await r.json();if(!r.ok)throw new Error(x.error||'Unable to load API tokens.');tokenList.replaceChildren();if(!x.tokens.length){tokenList.textContent='No active API tokens.';return}for(const token of x.tokens){const row=document.createElement('div'),summary=document.createElement('div'),button=document.createElement('button');row.className='token-row';summary.textContent=token.name+' — issued '+token.issued_at+' — final four '+token.final_four;button.type='button';button.className='danger';button.textContent='Delete '+token.name;button.onclick=()=>openTokenDelete(token);row.append(summary,button);tokenList.append(row)}}catch(error){tokenList.textContent=error.message||'Unable to load API tokens.'}}"
             "tokenForm.onsubmit=async e=>{e.preventDefault();tokenResult.textContent='Creating API token…';tokenOnce.hidden=true;tokenValue.textContent='';const body=new URLSearchParams(new FormData(tokenForm));try{const r=await fetch('/api/v1/admin/tokens',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-ESP32-NUT-CSRF':csrf},body}),x=await r.json();tokenResult.textContent=x.error||'';if(r.ok){tokenValue.textContent=x.token;tokenMetadata.textContent=x.name+' — issued '+x.issued_at+' — scope ota.install — final four '+x.final_four;tokenOnce.hidden=false;tokenResult.textContent='API token created.';tokenForm.reset();loadTokens()}}catch(error){tokenResult.textContent='Unable to reach the API-token service.'}};"
             "function openTokenDelete(token){pendingTokenId=token.id;deleteTokenName.textContent=token.name+' (final four '+token.final_four+')';deleteTokenAck.checked=false;deleteTokenConfirm.disabled=true;deleteTokenDialog.showModal()}"
             "deleteTokenAck.onchange=()=>deleteTokenConfirm.disabled=!deleteTokenAck.checked;document.getElementById('deleteTokenCancel').onclick=()=>deleteTokenDialog.close();deleteTokenDialog.addEventListener('close',()=>{pendingTokenId='';deleteTokenAck.checked=false;deleteTokenConfirm.disabled=true});"
             "deleteTokenConfirm.onclick=async()=>{if(!pendingTokenId||!deleteTokenAck.checked)return;const id=pendingTokenId;deleteTokenConfirm.disabled=true;tokenResult.textContent='Deleting API token…';try{const body=new URLSearchParams({id,acknowledge:'true'}),r=await fetch('/api/v1/admin/tokens',{method:'DELETE',headers:{'Content-Type':'application/x-www-form-urlencoded','X-ESP32-NUT-CSRF':csrf},body}),x=await r.json();tokenResult.textContent=x.message||x.error||'Token deletion failed.';if(r.ok){deleteTokenDialog.close();loadTokens()}else{deleteTokenConfirm.disabled=false}}catch(error){tokenResult.textContent='Unable to reach the API-token service.';deleteTokenConfirm.disabled=false}};"
             "otaCheckButton.onclick=async()=>{const file=otaFile.files[0];if(!file){otaResult.textContent='Choose a firmware .bin file first.';return}otaCheckButton.disabled=true;otaButton.disabled=true;otaResult.textContent='Checking firmware image…';try{const r=await fetch('/api/v1/ota/check',{method:'POST',headers:{'Content-Type':'application/octet-stream','X-ESP32-NUT-CSRF':csrf},body:file});const x=await r.json();otaResult.textContent=x.message||x.error||('Firmware check failed (HTTP '+r.status+').')}catch(error){otaResult.textContent='Unable to reach the firmware check service.'}finally{otaCheckButton.disabled=false;otaButton.disabled=false}};"
             "otaForm.onsubmit=async e=>{e.preventDefault();const file=otaFile.files[0];if(!file||!window.confirm('Install '+file.name+' and restart ESP32-NUT?'))return;otaButton.disabled=true;otaCheckButton.disabled=true;otaResult.textContent='Uploading and verifying firmware…';try{const r=await fetch('/api/v1/ota/install',{method:'POST',headers:{'Content-Type':'application/octet-stream','X-ESP32-NUT-CSRF':csrf},body:file});const x=await r.json();otaResult.textContent=x.message||x.error||('Firmware installation failed (HTTP '+r.status+').');if(r.ok){setTimeout(reconnect,5000)}else{otaButton.disabled=false;otaCheckButton.disabled=false}}catch(error){otaResult.textContent='Connection closed. The device may be restarting…';setTimeout(reconnect,3000)}};"
             "function reconnect(){fetch('/',{cache:'no-store'}).then(()=>location='/').catch(()=>setTimeout(reconnect,2000))}function logout(){fetch('/logout',{method:'POST',headers:{'X-ESP32-NUT-CSRF':csrf}}).then(()=>location='/')}loadStatus();loadTokens();</script>",
             csrf);
    mbedtls_platform_zeroize(csrf, sizeof(csrf));
    if (page_length < 0 || page_length >= MANAGEMENT_ADMIN_PAGE_SIZE)
    {
        mbedtls_platform_zeroize(page, MANAGEMENT_ADMIN_PAGE_SIZE);
        free(page);
        return management_send_html_status(
            request, "500 Internal Server Error",
            "<h1>ESP32-NUT administration</h1><p>The administration page exceeded its buffer.</p>");
    }
    const esp_err_t send_result = management_send_html(request, page);
    mbedtls_platform_zeroize(page, MANAGEMENT_ADMIN_PAGE_SIZE);
    free(page);
    return send_result;
}

static esp_err_t management_setup_handler(httpd_req_t *request)
{
    if (management_admin_password_is_configured())
    {
        return management_send_redirect(request, "/");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char password[129] = {0};
    char confirmation[129] = {0};
    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1] = {0};
    const esp_err_t form_result = management_read_form_body(request, body, sizeof(body));
    const bool fields_present = form_result == ESP_OK &&
                                management_form_value(body, "password", password, sizeof(password)) &&
                                management_form_value(body, "confirm", confirmation, sizeof(confirmation)) &&
                                management_form_value(body, "csrf", csrf, sizeof(csrf));
    const bool csrf_valid = fields_present && management_setup_csrf_is_valid(request, csrf);
    const bool matches = csrf_valid && strcmp(password, confirmation) == 0;
    const esp_err_t password_result = matches ? management_set_admin_password(password) : ESP_ERR_INVALID_ARG;
    mbedtls_platform_zeroize(body, sizeof(body));
    mbedtls_platform_zeroize(password, sizeof(password));
    mbedtls_platform_zeroize(confirmation, sizeof(confirmation));
    mbedtls_platform_zeroize(csrf, sizeof(csrf));
    if (!csrf_valid)
    {
        return management_send_html_status(request, "403 Forbidden",
                                           "<h1>ESP32-NUT setup</h1><p>The setup form expired or was invalid. <a href='/'>Start again</a>.</p>");
    }
    if (!matches)
    {
        return management_send_html_status(request, "400 Bad Request",
                                           "<h1>ESP32-NUT setup</h1><p>Passwords must match and contain 12 to 128 characters. <a href='/'>Try again</a>.</p>");
    }
    if (password_result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to store ADMIN password: %s", esp_err_to_name(password_result));
        return management_send_html_status(request, "500 Internal Server Error",
                                           "<h1>ESP32-NUT setup</h1><p>Unable to save the ADMIN password. <a href='/'>Try again</a>.</p>");
    }

    management_start_session();
    management_record_login_success();
    char session_header[176];
    management_set_session_cookie(request, session_header, sizeof(session_header));
    return management_send_redirect(request, "/");
}

static esp_err_t management_send_login_throttled(httpd_req_t *request, int retry_after)
{
    char retry_after_header[12];
    char page[1400];
    snprintf(retry_after_header, sizeof(retry_after_header), "%d", retry_after);
    httpd_resp_set_hdr(request, "Retry-After", retry_after_header);
    snprintf(page, sizeof(page),
             "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
             "<title>ESP32-NUT sign in paused</title><style>body{font:17px -apple-system,BlinkMacSystemFont,sans-serif;margin:2rem;max-width:32rem;color:#17212b}</style>"
             "<h1>ESP32-NUT sign in</h1><p>Too many failed attempts. Try again in <strong id=remaining>%d</strong> seconds.</p>"
             "<p>This page will reload automatically when sign-in is available.</p>"
             "<script>let remaining=%d;const output=document.getElementById('remaining');const timer=setInterval(()=>{remaining--;output.textContent=Math.max(remaining,0);if(remaining<=0){clearInterval(timer);location='/' }},1000)</script>",
             retry_after, retry_after);
    return management_send_html_status(request, "429 Too Many Requests", page);
}

static esp_err_t management_login_page_handler(httpd_req_t *request)
{
    return management_send_redirect(request, "/");
}

static esp_err_t management_login_handler(httpd_req_t *request)
{
    const int64_t now = esp_timer_get_time();
    const int retry_after = management_login_retry_after_seconds(now);
    if (retry_after > 0)
    {
        return management_send_login_throttled(request, retry_after);
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char password[129] = {0};
    bool needs_migration = false;
    const bool valid = management_read_form_body(request, body, sizeof(body)) == ESP_OK &&
                       management_form_value(body, "password", password, sizeof(password)) &&
                       management_verify_admin_password(password, &needs_migration);
    if (valid && needs_migration)
    {
        const esp_err_t migration_result = management_set_admin_password(password);
        if (migration_result != ESP_OK)
        {
            ESP_LOGE(TAG, "Unable to migrate ADMIN password credential: %s",
                     esp_err_to_name(migration_result));
        }
    }
    mbedtls_platform_zeroize(body, sizeof(body));
    mbedtls_platform_zeroize(password, sizeof(password));
    if (!valid)
    {
        if (management_record_login_failure(now))
        {
            return management_send_login_throttled(request,
                                                   (int)(MANAGEMENT_LOGIN_COOLDOWN_US / 1000000LL));
        }
        return management_send_html_status(request, "401 Unauthorized",
                                           "<h1>ESP32-NUT sign in</h1><p>Invalid password. <a href='/'>Try again</a>.</p>");
    }

    management_record_login_success();
    management_start_session();
    char session_header[176];
    management_set_session_cookie(request, session_header, sizeof(session_header));
    return management_send_redirect(request, "/");
}

static esp_err_t management_password_change_handler(httpd_req_t *request)
{
    if (!management_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden",
                                    "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char current_password[129] = {0};
    char new_password[129] = {0};
    char confirmation[129] = {0};
    const esp_err_t form_result = management_read_form_body(request, body, sizeof(body));
    const bool fields_present = form_result == ESP_OK &&
                                management_form_value(body, "current", current_password,
                                                      sizeof(current_password)) &&
                                management_form_value(body, "password", new_password,
                                                      sizeof(new_password)) &&
                                management_form_value(body, "confirm", confirmation,
                                                      sizeof(confirmation));
    mbedtls_platform_zeroize(body, sizeof(body));

    const bool new_password_valid = fields_present &&
                                    strlen(new_password) >= 12 &&
                                    strlen(new_password) <= 128 &&
                                    strcmp(new_password, confirmation) == 0;
    const bool current_password_valid = new_password_valid &&
                                        management_verify_admin_password(current_password, NULL);
    const bool password_changed = current_password_valid &&
                                  strcmp(current_password, new_password) != 0;
    esp_err_t password_result = ESP_ERR_INVALID_ARG;
    if (password_changed)
    {
        password_result = management_set_admin_password(new_password);
    }

    mbedtls_platform_zeroize(current_password, sizeof(current_password));
    mbedtls_platform_zeroize(new_password, sizeof(new_password));
    mbedtls_platform_zeroize(confirmation, sizeof(confirmation));

    if (!fields_present || !new_password_valid)
    {
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"New passwords must match and contain 12 to 128 characters.\"}");
    }
    if (!current_password_valid)
    {
        return management_send_json(request, "403 Forbidden",
                                    "{\"error\":\"The current ADMIN password is incorrect.\"}");
    }
    if (!password_changed)
    {
        return management_send_json(request, "400 Bad Request",
                                    "{\"error\":\"Choose a new password that differs from the current password.\"}");
    }
    if (password_result != ESP_OK)
    {
        return management_send_json(request, "500 Internal Server Error",
                                    "{\"error\":\"Unable to save the new ADMIN password.\"}");
    }

    management_start_session();
    char session_header[176];
    management_set_session_cookie(request, session_header, sizeof(session_header));
    return management_send_json(request, "200 OK",
                                "{\"message\":\"ADMIN password changed. The browser session was refreshed.\"}");
}

static esp_err_t management_logout_handler(httpd_req_t *request)
{
    if (!management_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden", "{\"error\":\"Invalid session or CSRF token.\"}");
    }
    management_clear_session();
    httpd_resp_set_hdr(request, "Set-Cookie",
                       "ESP32NUT_SESSION=; Path=/; Max-Age=0; Secure; HttpOnly; SameSite=Strict");
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
    if (!management_require_session(request))
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
    if (!management_csrf_is_valid(request))
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
    if (!management_require_session(request))
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
    ManagementNutSnapshot nut_snapshot;
    management_collect_nut_snapshot(&nut_snapshot);
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
    MANAGEMENT_JSON_APPEND("},\"nut\":{\"port\":3493,\"mode\":\"read-only\","
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

static esp_err_t management_token_list_handler(httpd_req_t *request)
{
    if (!management_require_session(request))
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
    if (!management_csrf_is_valid(request))
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
    if (!management_csrf_is_valid(request))
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
    if (!management_csrf_is_valid(request))
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
    if (!management_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden", "{\"error\":\"Invalid session or CSRF token.\"}");
    }
    return ota_install_from_request(request);
}

static esp_err_t management_ota_check_handler(httpd_req_t *request)
{
    if (!management_csrf_is_valid(request))
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
    management_clear_session();
    management_record_login_success();
    return result;
}

esp_err_t management_server_start(void)
{
    if (management_https_server != NULL)
    {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(management_load_or_create_certificate(), TAG,
                        "Unable to prepare HTTPS certificate");

    httpd_ssl_config_t configuration = HTTPD_SSL_CONFIG_DEFAULT();
    configuration.httpd.server_port = MANAGEMENT_HTTPS_PORT;
    configuration.httpd.stack_size = 12288;
    configuration.httpd.max_open_sockets = 4;
    configuration.httpd.max_uri_handlers = MANAGEMENT_HTTPS_ROUTE_CAPACITY;
    configuration.httpd.lru_purge_enable = true;
    configuration.servercert = management_certificate;
    configuration.servercert_len = management_certificate_length;
    configuration.prvtkey_pem = management_private_key;
    configuration.prvtkey_len = management_private_key_length;

    esp_err_t result = httpd_ssl_start(&management_https_server, &configuration);
    if (result != ESP_OK)
    {
        management_https_server = NULL;
        ESP_LOGE(TAG, "Unable to start the HTTPS management server: %s", esp_err_to_name(result));
        return result;
    }

    const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = management_root_handler};
    const httpd_uri_t setup = {.uri = "/setup", .method = HTTP_POST, .handler = management_setup_handler};
    const httpd_uri_t login_page = {.uri = "/login", .method = HTTP_GET, .handler = management_login_page_handler};
    const httpd_uri_t login = {.uri = "/login", .method = HTTP_POST, .handler = management_login_handler};
    const httpd_uri_t password = {.uri = "/api/v1/admin/password", .method = HTTP_POST, .handler = management_password_change_handler};
    const httpd_uri_t logout = {.uri = "/logout", .method = HTTP_POST, .handler = management_logout_handler};
    const httpd_uri_t status = {.uri = "/api/v1/status", .method = HTTP_GET, .handler = management_status_handler};
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
        &time_configuration, &ota_check, &ota, &token_list, &token_create, &token_delete,
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
