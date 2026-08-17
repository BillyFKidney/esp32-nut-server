/** @file management-certificates.c @brief Create and persist HTTPS certificate material. @see management-certificates.h, nvs.h, mbedtls/x509_crt.h, mbedtls/pk.h */
#include "management-certificates.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "mbedtls/pk.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/x509_crt.h"
#include "nvs.h"
#include "psa/crypto.h"

#define TAG "nut-management"

#define MANAGEMENT_CERTIFICATES_NAMESPACE "management"
#define MANAGEMENT_CERTIFICATE_KEY "https-cert"
#define MANAGEMENT_PRIVATE_KEY_KEY "https-key"
#define MANAGEMENT_CERTIFICATE_BUFFER_SIZE 2048U
#define MANAGEMENT_PRIVATE_KEY_BUFFER_SIZE 1024U

_Static_assert(sizeof(MANAGEMENT_CERTIFICATES_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "Management NVS namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_CERTIFICATE_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "HTTPS certificate NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_PRIVATE_KEY_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "HTTPS private-key NVS key exceeds the ESP-IDF limit");

static ManagementCertificateMaterial management_certificate_material;

static esp_err_t management_certificates_load_blob(const char *key, uint8_t **value,
                                                    size_t *value_length)
{
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(MANAGEMENT_CERTIFICATES_NAMESPACE, NVS_READONLY,
                                &handle);
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

    uint8_t *buffer = calloc(1, length + 1U);
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

static esp_err_t management_certificates_store(const uint8_t *certificate,
                                               size_t certificate_length,
                                               const uint8_t *private_key,
                                               size_t private_key_length)
{
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(MANAGEMENT_CERTIFICATES_NAMESPACE, NVS_READWRITE,
                                &handle);
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

static void management_certificates_clear_material(void)
{
    free((void *)management_certificate_material.certificate);
    management_certificate_material.certificate = NULL;
    management_certificate_material.certificate_length = 0;
    if (management_certificate_material.private_key != NULL)
    {
        mbedtls_platform_zeroize((void *)management_certificate_material.private_key,
                                 management_certificate_material.private_key_length);
    }
    free((void *)management_certificate_material.private_key);
    management_certificate_material.private_key = NULL;
    management_certificate_material.private_key_length = 0;
}

static esp_err_t management_certificates_generate(void)
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
        mbedtls_result = mbedtls_x509write_crt_set_serial_raw(&certificate_writer,
                                                              serial_bytes,
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
        const size_t certificate_length = strlen((char *)certificate) + 1U;
        const size_t private_key_length = strlen((char *)private_key) + 1U;
        result = management_certificates_store(certificate, certificate_length,
                                               private_key, private_key_length);
        if (result == ESP_OK)
        {
            management_certificate_material.certificate = certificate;
            management_certificate_material.certificate_length = certificate_length;
            management_certificate_material.private_key = private_key;
            management_certificate_material.private_key_length = private_key_length;
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

esp_err_t management_certificates_load_or_create(void)
{
    uint8_t *certificate = NULL;
    size_t certificate_length = 0;
    uint8_t *private_key = NULL;
    size_t private_key_length = 0;
    esp_err_t certificate_result = management_certificates_load_blob(
        MANAGEMENT_CERTIFICATE_KEY, &certificate, &certificate_length);
    esp_err_t key_result = management_certificates_load_blob(
        MANAGEMENT_PRIVATE_KEY_KEY, &private_key, &private_key_length);
    if (certificate_result == ESP_OK && key_result == ESP_OK)
    {
        management_certificates_clear_material();
        management_certificate_material.certificate = certificate;
        management_certificate_material.certificate_length = certificate_length;
        management_certificate_material.private_key = private_key;
        management_certificate_material.private_key_length = private_key_length;
        return ESP_OK;
    }

    free(certificate);
    if (private_key != NULL)
    {
        mbedtls_platform_zeroize(private_key, private_key_length);
    }
    free(private_key);
    management_certificates_clear_material();
    return management_certificates_generate();
}

const ManagementCertificateMaterial *management_certificates_get_material(void)
{
    if (management_certificate_material.certificate == NULL ||
        management_certificate_material.private_key == NULL)
    {
        return NULL;
    }
    return &management_certificate_material;
}
