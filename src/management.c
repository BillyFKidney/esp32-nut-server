#include "management.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
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

#define TAG "nut-management"

#define MANAGEMENT_NAMESPACE "management"
#define MANAGEMENT_ADMIN_SALT_KEY "admin-salt"
#define MANAGEMENT_ADMIN_HASH_KEY "admin-hash"
#define MANAGEMENT_CERTIFICATE_KEY "https-cert"
#define MANAGEMENT_PRIVATE_KEY_KEY "https-key"
#define MANAGEMENT_DEVICE_NAME_KEY "device-name"

#define MANAGEMENT_DEFAULT_DEVICE_NAME "ESP32-NUT"
#define MANAGEMENT_HTTPS_PORT 443
#define MANAGEMENT_PASSWORD_SALT_BYTES 16
#define MANAGEMENT_PASSWORD_HASH_BYTES 32
#define MANAGEMENT_PASSWORD_ITERATIONS 100000
#define MANAGEMENT_SESSION_BYTES 32
#define MANAGEMENT_SESSION_HEX_LENGTH (MANAGEMENT_SESSION_BYTES * 2)
#define MANAGEMENT_SESSION_IDLE_US (15LL * 60LL * 1000000LL)
#define MANAGEMENT_FORM_BODY_LIMIT 320
#define MANAGEMENT_CERTIFICATE_BUFFER_SIZE 2048
#define MANAGEMENT_PRIVATE_KEY_BUFFER_SIZE 1024
#define MANAGEMENT_LOGIN_MAX_FAILURES 5
#define MANAGEMENT_LOGIN_COOLDOWN_US (60LL * 1000000LL)

typedef struct
{
    bool active;
    char cookie[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    int64_t last_activity_us;
} ManagementSession;

static httpd_handle_t management_https_server;
static uint8_t *management_certificate;
static size_t management_certificate_length;
static uint8_t *management_private_key;
static size_t management_private_key_length;
static ManagementSession management_session;
static portMUX_TYPE management_session_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned int management_login_failures;
static int64_t management_login_cooldown_until_us;

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
        result = psa_key_derivation_input_bytes(&operation,
                                                PSA_KEY_DERIVATION_INPUT_PASSWORD,
                                                (const uint8_t *)password,
                                                strlen(password));
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_key_derivation_input_bytes(&operation,
                                                PSA_KEY_DERIVATION_INPUT_SALT,
                                                salt, MANAGEMENT_PASSWORD_SALT_BYTES);
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_key_derivation_input_integer(&operation,
                                                  PSA_KEY_DERIVATION_INPUT_COST,
                                                  MANAGEMENT_PASSWORD_ITERATIONS);
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

    uint8_t salt[MANAGEMENT_PASSWORD_SALT_BYTES];
    uint8_t hash[MANAGEMENT_PASSWORD_HASH_BYTES];
    esp_fill_random(salt, sizeof(salt));
    esp_err_t result = management_derive_password_hash(password, salt, hash);
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(hash, sizeof(hash));
        return result;
    }

    nvs_handle_t handle = 0;
    result = management_open_nvs(NVS_READWRITE, &handle);
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, MANAGEMENT_ADMIN_SALT_KEY, salt, sizeof(salt));
    }
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, MANAGEMENT_ADMIN_HASH_KEY, hash, sizeof(hash));
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    if (handle != 0)
    {
        nvs_close(handle);
    }
    mbedtls_platform_zeroize(hash, sizeof(hash));
    mbedtls_platform_zeroize(salt, sizeof(salt));
    return result;
}

static bool management_verify_admin_password(const char *password)
{
    uint8_t salt[MANAGEMENT_PASSWORD_SALT_BYTES];
    uint8_t stored_hash[MANAGEMENT_PASSWORD_HASH_BYTES];
    uint8_t candidate_hash[MANAGEMENT_PASSWORD_HASH_BYTES];
    nvs_handle_t handle = 0;
    if (password == NULL || management_open_nvs(NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    size_t salt_length = sizeof(salt);
    size_t hash_length = sizeof(stored_hash);
    const esp_err_t salt_result = nvs_get_blob(handle, MANAGEMENT_ADMIN_SALT_KEY,
                                               salt, &salt_length);
    const esp_err_t hash_result = nvs_get_blob(handle, MANAGEMENT_ADMIN_HASH_KEY,
                                               stored_hash, &hash_length);
    nvs_close(handle);
    if (salt_result != ESP_OK || hash_result != ESP_OK ||
        salt_length != sizeof(salt) || hash_length != sizeof(stored_hash) ||
        management_derive_password_hash(password, salt, candidate_hash) != ESP_OK)
    {
        mbedtls_platform_zeroize(salt, sizeof(salt));
        mbedtls_platform_zeroize(stored_hash, sizeof(stored_hash));
        mbedtls_platform_zeroize(candidate_hash, sizeof(candidate_hash));
        return false;
    }

    const bool matches = management_constant_time_equal(stored_hash, candidate_hash,
                                                        sizeof(stored_hash));
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
        for (size_t index = 0; pair[index] != '\0' && name_length + 1 < sizeof(decoded_name); index++)
        {
            if (pair[index] == '%' && pair[index + 1] != '\0' && pair[index + 2] != '\0')
            {
                char hexadecimal[3] = {pair[index + 1], pair[index + 2], '\0'};
                decoded_name[name_length++] = (char)strtol(hexadecimal, NULL, 16);
                index += 2;
            }
            else
            {
                decoded_name[name_length++] = pair[index] == '+' ? ' ' : pair[index];
            }
        }
        decoded_name[name_length] = '\0';
        if (strcmp(decoded_name, expected_name) != 0)
        {
            continue;
        }

        size_t value_length = 0;
        for (size_t index = 0; encoded_value[index] != '\0' && value_length + 1 < destination_size; index++)
        {
            if (encoded_value[index] == '%' && encoded_value[index + 1] != '\0' &&
                encoded_value[index + 2] != '\0')
            {
                char hexadecimal[3] = {encoded_value[index + 1], encoded_value[index + 2], '\0'};
                destination[value_length++] = (char)strtol(hexadecimal, NULL, 16);
                index += 2;
            }
            else
            {
                destination[value_length++] = encoded_value[index] == '+' ? ' ' : encoded_value[index];
            }
        }
        destination[value_length] = '\0';
        return true;
    }
    return false;
}

static esp_err_t management_read_form(httpd_req_t *request, char *password,
                                      size_t password_size, char *confirmation,
                                      size_t confirmation_size)
{
    if (request->content_len <= 0 || request->content_len > MANAGEMENT_FORM_BODY_LIMIT)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
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

    char password_body[sizeof(body)];
    char confirmation_body[sizeof(body)];
    snprintf(password_body, sizeof(password_body), "%s", body);
    snprintf(confirmation_body, sizeof(confirmation_body), "%s", body);
    const bool password_found = management_extract_form_value(password_body, "password",
                                                               password, password_size);
    const bool confirmation_found = management_extract_form_value(confirmation_body, "confirm",
                                                                   confirmation, confirmation_size);
    mbedtls_platform_zeroize(body, sizeof(body));
    mbedtls_platform_zeroize(password_body, sizeof(password_body));
    mbedtls_platform_zeroize(confirmation_body, sizeof(confirmation_body));
    return password_found && confirmation_found ? ESP_OK : ESP_ERR_INVALID_ARG;
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

static void management_clear_session(void)
{
    taskENTER_CRITICAL(&management_session_lock);
    mbedtls_platform_zeroize(&management_session, sizeof(management_session));
    taskEXIT_CRITICAL(&management_session_lock);
}

static bool management_cookie_is_authorized(httpd_req_t *request)
{
    size_t cookie_length = httpd_req_get_hdr_value_len(request, "Cookie");
    if (cookie_length == 0 || cookie_length > 256)
    {
        return false;
    }

    char cookie_header[257];
    if (httpd_req_get_hdr_value_str(request, "Cookie", cookie_header,
                                    sizeof(cookie_header)) != ESP_OK)
    {
        return false;
    }

    const char *prefix = "ESP32NUT_SESSION=";
    char *value = strstr(cookie_header, prefix);
    if (value == NULL)
    {
        return false;
    }
    value += strlen(prefix);
    if (strlen(value) < MANAGEMENT_SESSION_HEX_LENGTH)
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
    return authorized;
}

static bool management_csrf_is_valid(httpd_req_t *request)
{
    if (!management_cookie_is_authorized(request))
    {
        return false;
    }

    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    if (httpd_req_get_hdr_value_str(request, "X-ESP32-NUT-CSRF", csrf,
                                    sizeof(csrf)) != ESP_OK)
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

static const char management_setup_page[] =
    "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>ESP32-NUT setup</title><style>body{font:17px -apple-system,BlinkMacSystemFont,sans-serif;margin:2rem;max-width:42rem;color:#17212b}input,button{font:inherit;padding:.75rem;width:100%;box-sizing:border-box;margin:.35rem 0 1rem}button{background:#267747;color:white;border:0;border-radius:.4rem;font-weight:600}.hint{color:#52606d}.check{display:flex;gap:.5rem;align-items:center}.check input{width:auto;margin:0}</style>"
    "<h1>ESP32-NUT administrator setup</h1><p>Choose the unique ADMIN password for this device. It is never stored in plaintext.</p>"
    "<form method=post action=/setup><label>ADMIN password<input id=password name=password type=password required minlength=12 maxlength=128 autocomplete=new-password></label>"
    "<label>Confirm ADMIN password<input id=confirm name=confirm type=password required minlength=12 maxlength=128 autocomplete=new-password></label>"
    "<label class=check><input id=show type=checkbox> Show password</label><button type=submit>Save administrator password</button></form>"
    "<p class=hint>Use at least 12 characters. A physical recovery action can return the device to this setup screen.</p>"
    "<script>show.onchange=()=>{password.type=confirm.type=show.checked?'text':'password'}</script>";

static const char management_login_page[] =
    "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>ESP32-NUT sign in</title><style>body{font:17px -apple-system,BlinkMacSystemFont,sans-serif;margin:2rem;max-width:32rem;color:#17212b}input,button{font:inherit;padding:.75rem;width:100%;box-sizing:border-box;margin:.35rem 0 1rem}button{background:#267747;color:white;border:0;border-radius:.4rem;font-weight:600}.check{display:flex;gap:.5rem;align-items:center}.check input{width:auto;margin:0}</style>"
    "<h1>ESP32-NUT</h1><p>Sign in as ADMIN.</p><form method=post action=/login><label>ADMIN password<input id=password name=password type=password required autocomplete=current-password></label>"
    "<input name=confirm type=hidden value=login><label class=check><input id=show type=checkbox> Show password</label><button type=submit>Sign in</button></form>"
    "<script>show.onchange=()=>password.type=show.checked?'text':'password'</script>";

static esp_err_t management_root_handler(httpd_req_t *request)
{
    if (!management_admin_password_is_configured())
    {
        return management_send_html(request, management_setup_page);
    }
    if (!management_cookie_is_authorized(request))
    {
        return management_send_html(request, management_login_page);
    }

    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    taskENTER_CRITICAL(&management_session_lock);
    snprintf(csrf, sizeof(csrf), "%s", management_session.csrf);
    taskEXIT_CRITICAL(&management_session_lock);

    char page[2300];
    snprintf(page, sizeof(page),
             "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
             "<title>ESP32-NUT administration</title><style>body{font:17px -apple-system,BlinkMacSystemFont,sans-serif;margin:2rem;max-width:48rem;color:#17212b}pre{background:#f0f3f5;padding:1rem;overflow:auto}button{font:inherit;padding:.7rem;background:#267747;color:white;border:0;border-radius:.4rem}</style>"
             "<h1>ESP32-NUT administration</h1><p>HTTPS is active with this device's self-signed certificate."
             " The administration API is LAN-only.</p><h2>Device status</h2><pre id=status>Loading…</pre>"
             "<p>OTA installation, Wi-Fi changes, API tokens, and additional diagnostics are being added to this authenticated console.</p>"
             "<button onclick=logout()>Sign out</button><script>const csrf='%s';fetch('/api/v1/status').then(r=>r.json()).then(x=>status.textContent=JSON.stringify(x,null,2));function logout(){fetch('/logout',{method:'POST',headers:{'X-ESP32-NUT-CSRF':csrf}}).then(()=>location='/')}</script>",
             csrf);
    return management_send_html(request, page);
}

static esp_err_t management_setup_handler(httpd_req_t *request)
{
    if (management_admin_password_is_configured())
    {
        return management_send_redirect(request, "/");
    }

    char password[129];
    char confirmation[129];
    const esp_err_t form_result = management_read_form(request, password, sizeof(password),
                                                       confirmation, sizeof(confirmation));
    const bool matches = form_result == ESP_OK && strcmp(password, confirmation) == 0;
    const esp_err_t password_result = matches ? management_set_admin_password(password) : ESP_ERR_INVALID_ARG;
    mbedtls_platform_zeroize(password, sizeof(password));
    mbedtls_platform_zeroize(confirmation, sizeof(confirmation));
    if (password_result != ESP_OK)
    {
        return management_send_html(request,
                                    "<h1>ESP32-NUT setup</h1><p>Passwords must match and contain 12 to 128 characters. <a href='/'>Try again</a>.</p>");
    }

    management_start_session();
    char session_header[192];
    taskENTER_CRITICAL(&management_session_lock);
    snprintf(session_header, sizeof(session_header),
             "ESP32NUT_SESSION=%s; Path=/; Max-Age=900; Secure; HttpOnly; SameSite=Strict",
             management_session.cookie);
    taskEXIT_CRITICAL(&management_session_lock);
    httpd_resp_set_hdr(request, "Set-Cookie", session_header);
    return management_send_redirect(request, "/");
}

static esp_err_t management_login_handler(httpd_req_t *request)
{
    const int64_t now = esp_timer_get_time();
    if (now < management_login_cooldown_until_us)
    {
        return management_send_html(request,
                                    "<h1>ESP32-NUT sign in</h1><p>Too many failed attempts. Wait one minute and try again.</p>");
    }

    char password[129];
    char ignored_confirmation[129];
    const bool valid = management_read_form(request, password, sizeof(password),
                                            ignored_confirmation, sizeof(ignored_confirmation)) == ESP_OK &&
                       management_verify_admin_password(password);
    mbedtls_platform_zeroize(password, sizeof(password));
    mbedtls_platform_zeroize(ignored_confirmation, sizeof(ignored_confirmation));
    if (!valid)
    {
        management_login_failures++;
        if (management_login_failures >= MANAGEMENT_LOGIN_MAX_FAILURES)
        {
            management_login_failures = 0;
            management_login_cooldown_until_us = now + MANAGEMENT_LOGIN_COOLDOWN_US;
        }
        return management_send_html(request,
                                    "<h1>ESP32-NUT sign in</h1><p>Invalid password. <a href='/'>Try again</a>.</p>");
    }

    management_login_failures = 0;
    management_start_session();
    char session_header[192];
    taskENTER_CRITICAL(&management_session_lock);
    snprintf(session_header, sizeof(session_header),
             "ESP32NUT_SESSION=%s; Path=/; Max-Age=900; Secure; HttpOnly; SameSite=Strict",
             management_session.cookie);
    taskEXIT_CRITICAL(&management_session_lock);
    httpd_resp_set_hdr(request, "Set-Cookie", session_header);
    return management_send_redirect(request, "/");
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
    char address[16] = "unassigned";
    if (ip_info.ip.addr != 0)
    {
        snprintf(address, sizeof(address), IPSTR, IP2STR(&ip_info.ip));
    }

    char response[640];
    snprintf(response, sizeof(response),
             "{\"device_name\":\"%s\",\"firmware\":\"%s\",\"uptime_seconds\":%lld,"
             "\"wifi\":{\"ip\":\"%s\",\"ssid\":\"%s\",\"rssi_dbm\":%d,\"connected\":%s},"
             "\"management\":{\"transport\":\"https\",\"certificate\":\"self-signed\",\"role\":\"ADMIN\"},"
             "\"nut\":{\"port\":3493,\"mode\":\"read-only\"}}",
             MANAGEMENT_DEFAULT_DEVICE_NAME,
             app_description != NULL ? app_description->version : "unknown",
             (long long)(esp_timer_get_time() / 1000000LL), address,
             access_point_result == ESP_OK ? (const char *)access_point.ssid : "",
             access_point_result == ESP_OK ? access_point.rssi : 0,
             access_point_result == ESP_OK ? "true" : "false");
    return management_send_json(request, "200 OK", response);
}

static esp_err_t management_ota_install_handler(httpd_req_t *request)
{
    if (!management_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden", "{\"error\":\"Invalid session or CSRF token.\"}");
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
    configuration.httpd.max_uri_handlers = 8;
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
    const httpd_uri_t login = {.uri = "/login", .method = HTTP_POST, .handler = management_login_handler};
    const httpd_uri_t logout = {.uri = "/logout", .method = HTTP_POST, .handler = management_logout_handler};
    const httpd_uri_t status = {.uri = "/api/v1/status", .method = HTTP_GET, .handler = management_status_handler};
    const httpd_uri_t ota = {.uri = "/api/v1/ota/install", .method = HTTP_POST, .handler = management_ota_install_handler};
    const httpd_uri_t *routes[] = {&root, &setup, &login, &logout, &status, &ota};
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
