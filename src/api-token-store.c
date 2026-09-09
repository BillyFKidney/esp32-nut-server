/** @file api-token-store.c @brief Private fixed-capacity API-token NVS persistence. */
#include "api-token-store.h"

#include <stdio.h>
#include <string.h>

#include "esp_random.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"
#include "psa/crypto.h"

#define API_TOKEN_NAMESPACE "management"
#define API_TOKEN_VALID_EPOCH ((time_t)1704067200)
#define API_TOKEN_VERIFIER_BYTES 32U
#define API_TOKEN_SALT_BYTES 16U
#define API_TOKEN_MAX_ID_ATTEMPTS 8U

typedef struct
{
    uint32_t version;
    uint32_t record_size;
    ApiTokenStoreRecord records[];
} ApiTokenStore;

static const uint8_t api_token_verifier_domain[] =
    "ESP32-NUT API token verifier v1";

static void api_token_store_bytes_to_hex(const uint8_t *source,
                                         size_t source_length, char *destination,
                                         size_t destination_length)
{
    static const char hexadecimal[] = "0123456789abcdef";
    if (destination == NULL || destination_length < source_length * 2U + 1U)
    {
        if (destination != NULL && destination_length > 0U)
        {
            destination[0] = '\0';
        }
        return;
    }

    for (size_t index = 0; index < source_length; index++)
    {
        destination[index * 2U] = hexadecimal[source[index] >> 4U];
        destination[index * 2U + 1U] = hexadecimal[source[index] & 0x0fU];
    }
    destination[source_length * 2U] = '\0';
}

static int api_token_store_hexadecimal_value(char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    return -1;
}

static bool api_token_store_hex_to_bytes(const char *source, size_t source_length,
                                         uint8_t *destination,
                                         size_t destination_length)
{
    if (source == NULL || destination == NULL ||
        source_length != destination_length * 2U)
    {
        return false;
    }
    for (size_t index = 0; index < destination_length; index++)
    {
        const int high = api_token_store_hexadecimal_value(source[index * 2U]);
        const int low = api_token_store_hexadecimal_value(source[index * 2U + 1U]);
        if (high < 0 || low < 0)
        {
            return false;
        }
        destination[index] = (uint8_t)((high << 4) | low);
    }
    return true;
}

static bool api_token_store_constant_time_equal(const uint8_t *left,
                                                 const uint8_t *right,
                                                 size_t length)
{
    uint8_t difference = 0;
    for (size_t index = 0; index < length; index++)
    {
        difference |= left[index] ^ right[index];
    }
    return difference == 0U;
}

static bool api_token_store_value_is_valid(const char *token)
{
    if (token == NULL || strlen(token) != API_TOKEN_VALUE_LENGTH ||
        strncmp(token, API_TOKEN_PREFIX, sizeof(API_TOKEN_PREFIX) - 1U) != 0)
    {
        return false;
    }
    for (size_t index = sizeof(API_TOKEN_PREFIX) - 1U;
         index < API_TOKEN_VALUE_LENGTH; index++)
    {
        if (api_token_store_hexadecimal_value(token[index]) < 0)
        {
            return false;
        }
    }
    return true;
}

bool api_token_store_name_is_valid(const char *name)
{
    if (name == NULL)
    {
        return false;
    }
    const size_t length = strlen(name);
    if (length == 0U || length > API_TOKEN_NAME_MAX_LENGTH || name[0] == ' ' ||
        name[length - 1U] == ' ')
    {
        return false;
    }

    for (size_t index = 0; index < length; index++)
    {
        const char character = name[index];
        if (!((character >= 'a' && character <= 'z') ||
              (character >= 'A' && character <= 'Z') ||
              (character >= '0' && character <= '9') || character == ' ' ||
              character == '-' || character == '_' || character == '.'))
        {
            return false;
        }
    }
    return true;
}

static char api_token_store_ascii_lower(char character)
{
    return character >= 'A' && character <= 'Z'
               ? (char)(character - 'A' + 'a')
               : character;
}

static bool api_token_store_names_equal(const char *left, const char *right)
{
    size_t index = 0;
    while (left[index] != '\0' && right[index] != '\0')
    {
        if (api_token_store_ascii_lower(left[index]) !=
            api_token_store_ascii_lower(right[index]))
        {
            return false;
        }
        index++;
    }
    return left[index] == right[index];
}

static void api_token_store_empty(ApiTokenStore *store,
                                  const ApiTokenStoreConfig *config,
                                  size_t storage_size)
{
    memset(store, 0, storage_size);
    store->version = config->version;
    store->record_size = sizeof(ApiTokenStoreRecord);
}

static bool api_token_store_record_is_valid(const ApiTokenStoreRecord *record,
                                            const ApiTokenStoreConfig *config)
{
    if (record->active == 0U)
    {
        return true;
    }
    uint8_t identifier_aggregate = 0U;
    for (size_t index = 0; index < API_TOKEN_ID_BYTES; index++)
    {
        identifier_aggregate |= record->id[index];
    }
    if (record->active != 1U || identifier_aggregate == 0U ||
        record->scopes != config->scope ||
        record->issued_at < (int64_t)API_TOKEN_VALID_EPOCH ||
        record->name[API_TOKEN_NAME_MAX_LENGTH] != '\0' ||
        !api_token_store_name_is_valid(record->name) ||
        record->final_four[API_TOKEN_FINAL_FOUR_LENGTH] != '\0' ||
        strlen(record->final_four) != API_TOKEN_FINAL_FOUR_LENGTH)
    {
        return false;
    }
    for (size_t index = 0; index < API_TOKEN_FINAL_FOUR_LENGTH; index++)
    {
        if (api_token_store_hexadecimal_value(record->final_four[index]) < 0)
        {
            return false;
        }
    }
    return true;
}

static esp_err_t api_token_store_load(const ApiTokenStoreConfig *config,
                                      ApiTokenStore *store, size_t storage_size)
{
    if (config == NULL || store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    api_token_store_empty(store, config, storage_size);

    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(API_TOKEN_NAMESPACE, NVS_READONLY, &handle);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        return ESP_OK;
    }
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(store, storage_size);
        return result;
    }

    size_t length = storage_size;
    result = nvs_get_blob(handle, config->nvs_key, store, &length);
    nvs_close(handle);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        api_token_store_empty(store, config, storage_size);
        return ESP_OK;
    }
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(store, storage_size);
        return result;
    }
    if (length != storage_size || store->version != config->version ||
        store->record_size != sizeof(ApiTokenStoreRecord))
    {
        mbedtls_platform_zeroize(store, storage_size);
        return ESP_ERR_INVALID_VERSION;
    }

    for (size_t index = 0; index < config->record_count; index++)
    {
        if (!api_token_store_record_is_valid(&store->records[index], config))
        {
            mbedtls_platform_zeroize(store, storage_size);
            return ESP_ERR_INVALID_CRC;
        }
        for (size_t other = index + 1U; other < config->record_count; other++)
        {
            if (store->records[index].active == 1U &&
                store->records[other].active == 1U &&
                (api_token_store_names_equal(store->records[index].name,
                                             store->records[other].name) ||
                 api_token_store_constant_time_equal(store->records[index].id,
                                                     store->records[other].id,
                                                     API_TOKEN_ID_BYTES)))
            {
                mbedtls_platform_zeroize(store, storage_size);
                return ESP_ERR_INVALID_CRC;
            }
        }
    }
    return ESP_OK;
}

static esp_err_t api_token_store_save(const ApiTokenStoreConfig *config,
                                      const ApiTokenStore *store,
                                      size_t storage_size)
{
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(API_TOKEN_NAMESPACE, NVS_READWRITE, &handle);
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, config->nvs_key, store, storage_size);
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

static esp_err_t api_token_store_derive_verifier(const char *token,
                                                  const uint8_t *salt,
                                                  uint8_t *verifier)
{
    psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
    psa_status_t result = psa_crypto_init();
    if (result == PSA_SUCCESS)
    {
        result = psa_hash_setup(&operation, PSA_ALG_SHA_256);
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_hash_update(&operation, api_token_verifier_domain,
                                 sizeof(api_token_verifier_domain) - 1U);
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_hash_update(&operation, salt, API_TOKEN_SALT_BYTES);
    }
    if (result == PSA_SUCCESS)
    {
        result = psa_hash_update(&operation, (const uint8_t *)token,
                                 API_TOKEN_VALUE_LENGTH);
    }
    size_t verifier_length = 0;
    if (result == PSA_SUCCESS)
    {
        result = psa_hash_finish(&operation, verifier, API_TOKEN_VERIFIER_BYTES,
                                 &verifier_length);
    }
    psa_hash_abort(&operation);
    return result == PSA_SUCCESS && verifier_length == API_TOKEN_VERIFIER_BYTES
               ? ESP_OK
               : ESP_FAIL;
}

static bool api_token_store_identifier_is_zero(const uint8_t *identifier)
{
    uint8_t aggregate = 0;
    for (size_t index = 0; index < API_TOKEN_ID_BYTES; index++)
    {
        aggregate |= identifier[index];
    }
    return aggregate == 0U;
}

static bool api_token_store_identifier_is_unique(const ApiTokenStore *store,
                                                 const ApiTokenStoreConfig *config,
                                                 const uint8_t *identifier)
{
    for (size_t index = 0; index < config->record_count; index++)
    {
        if (store->records[index].active == 1U &&
            api_token_store_constant_time_equal(store->records[index].id,
                                                identifier, API_TOKEN_ID_BYTES))
        {
            return false;
        }
    }
    return true;
}

static bool api_token_store_name_exists(const ApiTokenStore *store,
                                        const ApiTokenStoreConfig *config,
                                        const char *name)
{
    for (size_t index = 0; index < config->record_count; index++)
    {
        if (store->records[index].active == 1U &&
            api_token_store_names_equal(store->records[index].name, name))
        {
            return true;
        }
    }
    return false;
}

static bool api_token_store_format_issued_at(
    int64_t issued_at, char destination[API_TOKEN_ISSUED_AT_LENGTH + 1U])
{
    const time_t timestamp = (time_t)issued_at;
    struct tm utc = {0};
    return gmtime_r(&timestamp, &utc) != NULL &&
           strftime(destination, API_TOKEN_ISSUED_AT_LENGTH + 1U,
                    "%Y-%m-%dT%H:%M:%SZ", &utc) == API_TOKEN_ISSUED_AT_LENGTH;
}

static bool api_token_store_metadata_from_record(const ApiTokenStoreRecord *record,
                                                 ApiTokenMetadata *metadata)
{
    memset(metadata, 0, sizeof(*metadata));
    api_token_store_bytes_to_hex(record->id, sizeof(record->id), metadata->id,
                                 sizeof(metadata->id));
    snprintf(metadata->name, sizeof(metadata->name), "%s", record->name);
    snprintf(metadata->final_four, sizeof(metadata->final_four), "%s",
             record->final_four);
    metadata->scopes = record->scopes;
    return api_token_store_format_issued_at(record->issued_at, metadata->issued_at);
}

esp_err_t api_token_store_list(const ApiTokenStoreConfig *config, void *storage,
                               size_t storage_size, ApiTokenMetadata *metadata,
                               size_t metadata_capacity, size_t *metadata_count)
{
    if (config == NULL || storage == NULL || metadata == NULL ||
        metadata_count == NULL || metadata_capacity < config->record_count)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(metadata, 0, sizeof(*metadata) * metadata_capacity);
    *metadata_count = 0U;

    ApiTokenStore *store = storage;
    esp_err_t result = api_token_store_load(config, store, storage_size);
    if (result == ESP_OK)
    {
        for (size_t index = 0; index < config->record_count; index++)
        {
            if (store->records[index].active == 1U)
            {
                if (!api_token_store_metadata_from_record(
                        &store->records[index], &metadata[*metadata_count]))
                {
                    result = ESP_ERR_INVALID_STATE;
                    break;
                }
                (*metadata_count)++;
            }
        }
    }
    mbedtls_platform_zeroize(store, storage_size);
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(metadata, sizeof(*metadata) * metadata_capacity);
        *metadata_count = 0U;
    }
    return result;
}

esp_err_t api_token_store_create(const ApiTokenStoreConfig *config, void *storage,
                                 size_t storage_size, const char *name,
                                 time_t issued_at, ApiTokenMetadata *metadata,
                                 char token[API_TOKEN_VALUE_LENGTH + 1U])
{
    if (config == NULL || storage == NULL || !api_token_store_name_is_valid(name) ||
        issued_at < API_TOKEN_VALID_EPOCH || metadata == NULL || token == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(metadata, 0, sizeof(*metadata));
    memset(token, 0, API_TOKEN_VALUE_LENGTH + 1U);

    ApiTokenStore *store = storage;
    esp_err_t result = api_token_store_load(config, store, storage_size);
    if (result != ESP_OK)
    {
        return result;
    }
    if (api_token_store_name_exists(store, config, name))
    {
        mbedtls_platform_zeroize(store, storage_size);
        return ESP_ERR_INVALID_STATE;
    }

    size_t available_index = config->record_count;
    for (size_t index = 0; index < config->record_count; index++)
    {
        if (store->records[index].active == 0U)
        {
            available_index = index;
            break;
        }
    }
    if (available_index == config->record_count)
    {
        mbedtls_platform_zeroize(store, storage_size);
        return ESP_ERR_NO_MEM;
    }

    ApiTokenStoreRecord *record = &store->records[available_index];
    memset(record, 0, sizeof(*record));
    bool identifier_ready = false;
    for (size_t attempt = 0;
         attempt < API_TOKEN_MAX_ID_ATTEMPTS && !identifier_ready; attempt++)
    {
        esp_fill_random(record->id, sizeof(record->id));
        identifier_ready = !api_token_store_identifier_is_zero(record->id) &&
                           api_token_store_identifier_is_unique(store, config,
                                                                record->id);
    }
    if (!identifier_ready)
    {
        mbedtls_platform_zeroize(store, storage_size);
        return ESP_FAIL;
    }

    uint8_t random_value[API_TOKEN_RANDOM_BYTES];
    esp_fill_random(random_value, sizeof(random_value));
    memcpy(token, API_TOKEN_PREFIX, sizeof(API_TOKEN_PREFIX) - 1U);
    api_token_store_bytes_to_hex(random_value, sizeof(random_value),
                                 token + sizeof(API_TOKEN_PREFIX) - 1U,
                                 API_TOKEN_RANDOM_BYTES * 2U + 1U);
    esp_fill_random(record->salt, sizeof(record->salt));
    result = api_token_store_derive_verifier(token, record->salt, record->verifier);
    mbedtls_platform_zeroize(random_value, sizeof(random_value));
    if (result == ESP_OK)
    {
        record->issued_at = (int64_t)issued_at;
        record->scopes = config->scope;
        record->active = 1U;
        snprintf(record->name, sizeof(record->name), "%s", name);
        snprintf(record->final_four, sizeof(record->final_four), "%s",
                 token + API_TOKEN_VALUE_LENGTH - API_TOKEN_FINAL_FOUR_LENGTH);
        result = api_token_store_metadata_from_record(record, metadata)
                     ? api_token_store_save(config, store, storage_size)
                     : ESP_ERR_INVALID_STATE;
    }
    mbedtls_platform_zeroize(store, storage_size);
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(metadata, sizeof(*metadata));
        mbedtls_platform_zeroize(token, API_TOKEN_VALUE_LENGTH + 1U);
    }
    return result;
}

esp_err_t api_token_store_delete(const ApiTokenStoreConfig *config, void *storage,
                                 size_t storage_size, const char *id)
{
    if (config == NULL || storage == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t identifier[API_TOKEN_ID_BYTES] = {0};
    if (id == NULL || strlen(id) != API_TOKEN_ID_HEX_LENGTH ||
        !api_token_store_hex_to_bytes(id, API_TOKEN_ID_HEX_LENGTH, identifier,
                                      sizeof(identifier)))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ApiTokenStore *store = storage;
    esp_err_t result = api_token_store_load(config, store, storage_size);
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(identifier, sizeof(identifier));
        return result;
    }
    size_t found_index = config->record_count;
    for (size_t index = 0; index < config->record_count; index++)
    {
        if (store->records[index].active == 1U &&
            api_token_store_constant_time_equal(store->records[index].id, identifier,
                                                sizeof(identifier)))
        {
            found_index = index;
            break;
        }
    }
    mbedtls_platform_zeroize(identifier, sizeof(identifier));
    if (found_index == config->record_count)
    {
        mbedtls_platform_zeroize(store, storage_size);
        return ESP_ERR_NOT_FOUND;
    }
    for (size_t index = found_index; index + 1U < config->record_count; index++)
    {
        store->records[index] = store->records[index + 1U];
    }
    mbedtls_platform_zeroize(&store->records[config->record_count - 1U],
                             sizeof(store->records[0]));
    result = api_token_store_save(config, store, storage_size);
    mbedtls_platform_zeroize(store, storage_size);
    return result;
}

bool api_token_store_authorize(const ApiTokenStoreConfig *config, void *storage,
                               size_t storage_size, const char *token,
                               uint32_t required_scope)
{
    if (config == NULL || storage == NULL || !api_token_store_value_is_valid(token) ||
        required_scope == 0U || (required_scope & ~config->scope) != 0U)
    {
        return false;
    }

    ApiTokenStore *store = storage;
    if (api_token_store_load(config, store, storage_size) != ESP_OK)
    {
        return false;
    }
    uint8_t authorized = 0U;
    uint8_t candidate[API_TOKEN_VERIFIER_BYTES] = {0};
    for (size_t index = 0; index < config->record_count; index++)
    {
        const ApiTokenStoreRecord *record = &store->records[index];
        if (api_token_store_derive_verifier(token, record->salt, candidate) != ESP_OK)
        {
            authorized = 0U;
            break;
        }
        const uint8_t verifier_matches =
            api_token_store_constant_time_equal(candidate, record->verifier,
                                                sizeof(candidate))
                ? 1U
                : 0U;
        const uint8_t record_active = record->active == 1U ? 1U : 0U;
        const uint8_t scope_matches =
            (record->scopes & required_scope) == required_scope ? 1U : 0U;
        authorized |= verifier_matches & record_active & scope_matches;
        mbedtls_platform_zeroize(candidate, sizeof(candidate));
    }
    mbedtls_platform_zeroize(candidate, sizeof(candidate));
    mbedtls_platform_zeroize(store, storage_size);
    return authorized != 0U;
}
