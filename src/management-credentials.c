/** @file management-credentials.c @brief Store and verify ADMIN password credentials. @see management-credentials.h, nvs.h, psa/crypto.h, mbedtls/platform_util.h */
#include "management-credentials.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_random.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"
#include "psa/crypto.h"

#define MANAGEMENT_CREDENTIALS_NAMESPACE "management"
#define MANAGEMENT_ADMIN_SALT_KEY "admin-salt"
#define MANAGEMENT_ADMIN_HASH_KEY "admin-hash"
#define MANAGEMENT_ADMIN_CREDENTIAL_KEY "admin-cred"
#define MANAGEMENT_ADMIN_STAGED_CREDENTIAL_KEY "admin-stage"

#define MANAGEMENT_PASSWORD_SALT_BYTES 16
#define MANAGEMENT_PASSWORD_HASH_BYTES 32
#define MANAGEMENT_PASSWORD_CREDENTIAL_VERSION 1U
#define MANAGEMENT_PASSWORD_ITERATIONS 12500U
#define MANAGEMENT_PASSWORD_LEGACY_ITERATIONS 100000U
#define MANAGEMENT_PASSWORD_MIN_ITERATIONS 1000U
#define MANAGEMENT_PASSWORD_MAX_ITERATIONS 1000000U

_Static_assert(sizeof(MANAGEMENT_CREDENTIALS_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "Management NVS namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_ADMIN_SALT_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "ADMIN salt NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_ADMIN_HASH_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "ADMIN hash NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_ADMIN_CREDENTIAL_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "ADMIN credential NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_ADMIN_STAGED_CREDENTIAL_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "Staged ADMIN credential NVS key exceeds the ESP-IDF limit");

typedef struct
{
    uint32_t version;
    uint32_t iterations;
    uint8_t salt[MANAGEMENT_PASSWORD_SALT_BYTES];
    uint8_t hash[MANAGEMENT_PASSWORD_HASH_BYTES];
} ManagementAdminCredential;

static bool management_credentials_is_current_format(
    const ManagementAdminCredential *credential, size_t credential_length)
{
    return credential != NULL &&
           credential_length == sizeof(*credential) &&
           credential->version == MANAGEMENT_PASSWORD_CREDENTIAL_VERSION &&
           credential->iterations >= MANAGEMENT_PASSWORD_MIN_ITERATIONS &&
           credential->iterations <= MANAGEMENT_PASSWORD_MAX_ITERATIONS;
}

static bool management_credentials_constant_time_equal(const uint8_t *left,
                                                        const uint8_t *right,
                                                        size_t length)
{
    uint8_t difference = 0;
    for (size_t index = 0; index < length; index++)
    {
        difference |= left[index] ^ right[index];
    }
    return difference == 0;
}

static esp_err_t management_credentials_open_nvs(nvs_open_mode_t mode,
                                                  nvs_handle_t *handle)
{
    return nvs_open(MANAGEMENT_CREDENTIALS_NAMESPACE, mode, handle);
}

bool management_admin_password_is_configured(void)
{
    nvs_handle_t handle = 0;
    if (management_credentials_open_nvs(NVS_READONLY, &handle) != ESP_OK)
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
        const bool valid = management_credentials_is_current_format(&credential,
                                                                     credential_length);
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

static esp_err_t management_credentials_derive_password_hash(const char *password,
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

static bool management_credentials_matches_password(const ManagementAdminCredential *credential,
                                                     const char *password)
{
    uint8_t candidate_hash[MANAGEMENT_PASSWORD_HASH_BYTES] = {0};
    const bool matches = management_credentials_is_current_format(credential,
                                                                    sizeof(*credential)) &&
                         password != NULL &&
                         management_credentials_derive_password_hash(
                             password, credential->salt, credential->iterations,
                             candidate_hash) == ESP_OK &&
                         management_credentials_constant_time_equal(
                             credential->hash, candidate_hash, sizeof(candidate_hash));
    mbedtls_platform_zeroize(candidate_hash, sizeof(candidate_hash));
    return matches;
}

esp_err_t management_credentials_set_admin_password(const char *password)
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
    esp_err_t result = management_credentials_derive_password_hash(
        password, credential.salt, credential.iterations, credential.hash);
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(&credential, sizeof(credential));
        return result;
    }

    nvs_handle_t handle = 0;
    result = management_credentials_open_nvs(NVS_READWRITE, &handle);
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, MANAGEMENT_ADMIN_STAGED_CREDENTIAL_KEY,
                              &credential, sizeof(credential));
    }
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    if (handle != 0)
    {
        nvs_close(handle);
        handle = 0;
    }
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(&credential, sizeof(credential));
        return result;
    }

    ManagementAdminCredential staged_credential = {0};
    size_t staged_credential_length = sizeof(staged_credential);
    result = management_credentials_open_nvs(NVS_READONLY, &handle);
    if (result == ESP_OK)
    {
        result = nvs_get_blob(handle, MANAGEMENT_ADMIN_STAGED_CREDENTIAL_KEY,
                              &staged_credential, &staged_credential_length);
    }
    if (handle != 0)
    {
        nvs_close(handle);
        handle = 0;
    }
    if (result != ESP_OK || !management_credentials_is_current_format(
                            &staged_credential, staged_credential_length) ||
        !management_credentials_matches_password(&staged_credential, password))
    {
        mbedtls_platform_zeroize(&staged_credential, sizeof(staged_credential));
        mbedtls_platform_zeroize(&credential, sizeof(credential));
        return result == ESP_OK ? ESP_FAIL : result;
    }
    mbedtls_platform_zeroize(&staged_credential, sizeof(staged_credential));

    result = management_credentials_open_nvs(NVS_READWRITE, &handle);
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
        result = nvs_erase_key(handle, MANAGEMENT_ADMIN_STAGED_CREDENTIAL_KEY);
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

bool management_credentials_verify_admin_password(const char *password,
                                                  bool *needs_migration)
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
    if (password == NULL || management_credentials_open_nvs(NVS_READONLY, &handle) != ESP_OK)
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
        management_credentials_derive_password_hash(password, salt, iterations,
                                                    candidate_hash) != ESP_OK)
    {
        mbedtls_platform_zeroize(salt, sizeof(salt));
        mbedtls_platform_zeroize(stored_hash, sizeof(stored_hash));
        mbedtls_platform_zeroize(candidate_hash, sizeof(candidate_hash));
        return false;
    }

    const bool matches = management_credentials_constant_time_equal(
        stored_hash, candidate_hash, sizeof(stored_hash));
    if (matches && legacy_loaded && needs_migration != NULL)
    {
        *needs_migration = true;
    }
    mbedtls_platform_zeroize(salt, sizeof(salt));
    mbedtls_platform_zeroize(stored_hash, sizeof(stored_hash));
    mbedtls_platform_zeroize(candidate_hash, sizeof(candidate_hash));
    return matches;
}
