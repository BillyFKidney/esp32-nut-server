#include "api_tokens.h"

#include <stdio.h>
#include <string.h>

#include "esp_random.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"
#include "psa/crypto.h"

#define API_TOKEN_NAMESPACE "management"
#define API_TOKEN_NVS_KEY "api-tokens"
#define API_TOKEN_STORE_VERSION 1U
#define API_TOKEN_VALID_EPOCH ((time_t)1704067200)
#define API_TOKEN_VERIFIER_BYTES 32U
#define API_TOKEN_SALT_BYTES 16U

typedef struct
{
    int64_t issued_at;
    uint32_t scopes;
    uint8_t active;
    uint8_t reserved_before_id[3];
    uint8_t id[API_TOKEN_ID_BYTES];
    uint8_t salt[API_TOKEN_SALT_BYTES];
    uint8_t verifier[API_TOKEN_VERIFIER_BYTES];
    char name[API_TOKEN_NAME_MAX_LENGTH + 1U];
    char final_four[API_TOKEN_FINAL_FOUR_LENGTH + 1U];
    uint8_t reserved_after_final_four[2];
} StoredApiToken;

typedef struct
{
    uint32_t version;
    uint32_t record_size;
    StoredApiToken records[API_TOKEN_MAX_COUNT];
} StoredApiTokenSet;

_Static_assert(sizeof(API_TOKEN_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "API-token NVS namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(API_TOKEN_NVS_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "API-token NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(StoredApiToken) == 112U,
               "API-token record layout changed; increment the store version");
_Static_assert(sizeof(StoredApiTokenSet) == 456U,
               "API-token store layout changed; increment the store version");

static const uint8_t api_token_verifier_domain[] =
    "ESP32-NUT API token verifier v1";

static void api_tokens_bytes_to_hex(const uint8_t *source, size_t source_length,
                                    char *destination, size_t destination_length)
{
    static const char hexadecimal[] = "0123456789abcdef";
    if (destination == NULL ||
        destination_length < source_length * 2U + 1U)
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

static int api_tokens_hexadecimal_value(char character)
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

static bool api_tokens_hex_to_bytes(const char *source, size_t source_length,
                                    uint8_t *destination, size_t destination_length)
{
    if (source == NULL || destination == NULL ||
        source_length != destination_length * 2U)
    {
        return false;
    }
    for (size_t index = 0; index < destination_length; index++)
    {
        const int high = api_tokens_hexadecimal_value(source[index * 2U]);
        const int low = api_tokens_hexadecimal_value(source[index * 2U + 1U]);
        if (high < 0 || low < 0)
        {
            return false;
        }
        destination[index] = (uint8_t)((high << 4) | low);
    }
    return true;
}

static bool api_tokens_constant_time_equal(const uint8_t *left,
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

static bool api_token_value_is_valid(const char *token)
{
    if (token == NULL || strlen(token) != API_TOKEN_VALUE_LENGTH ||
        strncmp(token, API_TOKEN_PREFIX, sizeof(API_TOKEN_PREFIX) - 1U) != 0)
    {
        return false;
    }
    for (size_t index = sizeof(API_TOKEN_PREFIX) - 1U;
         index < API_TOKEN_VALUE_LENGTH; index++)
    {
        if (api_tokens_hexadecimal_value(token[index]) < 0)
        {
            return false;
        }
    }
    return true;
}

bool api_token_name_is_valid(const char *name)
{
    if (name == NULL)
    {
        return false;
    }
    const size_t length = strlen(name);
    if (length == 0U || length > API_TOKEN_NAME_MAX_LENGTH ||
        name[0] == ' ' || name[length - 1U] == ' ')
    {
        return false;
    }

    for (size_t index = 0; index < length; index++)
    {
        const char character = name[index];
        if (!((character >= 'a' && character <= 'z') ||
              (character >= 'A' && character <= 'Z') ||
              (character >= '0' && character <= '9') ||
              character == ' ' || character == '-' || character == '_' ||
              character == '.'))
        {
            return false;
        }
    }
    return true;
}

static char api_tokens_ascii_lower(char character)
{
    return character >= 'A' && character <= 'Z'
               ? (char)(character - 'A' + 'a')
               : character;
}

static bool api_tokens_names_equal(const char *left, const char *right)
{
    size_t index = 0;
    while (left[index] != '\0' && right[index] != '\0')
    {
        if (api_tokens_ascii_lower(left[index]) !=
            api_tokens_ascii_lower(right[index]))
        {
            return false;
        }
        index++;
    }
    return left[index] == right[index];
}

static void api_tokens_empty_store(StoredApiTokenSet *store)
{
    memset(store, 0, sizeof(*store));
    store->version = API_TOKEN_STORE_VERSION;
    store->record_size = sizeof(StoredApiToken);
}

static bool api_tokens_record_is_valid(const StoredApiToken *record)
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
        record->scopes != API_TOKEN_SCOPE_OTA_INSTALL ||
        record->issued_at < (int64_t)API_TOKEN_VALID_EPOCH ||
        record->name[API_TOKEN_NAME_MAX_LENGTH] != '\0' ||
        !api_token_name_is_valid(record->name) ||
        record->final_four[API_TOKEN_FINAL_FOUR_LENGTH] != '\0' ||
        strlen(record->final_four) != API_TOKEN_FINAL_FOUR_LENGTH)
    {
        return false;
    }
    for (size_t index = 0; index < API_TOKEN_FINAL_FOUR_LENGTH; index++)
    {
        if (api_tokens_hexadecimal_value(record->final_four[index]) < 0)
        {
            return false;
        }
    }
    return true;
}

static esp_err_t api_tokens_load(StoredApiTokenSet *store)
{
    if (store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    api_tokens_empty_store(store);

    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(API_TOKEN_NAMESPACE, NVS_READONLY, &handle);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        return ESP_OK;
    }
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(store, sizeof(*store));
        return result;
    }

    size_t length = sizeof(*store);
    result = nvs_get_blob(handle, API_TOKEN_NVS_KEY, store, &length);
    nvs_close(handle);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        api_tokens_empty_store(store);
        return ESP_OK;
    }
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(store, sizeof(*store));
        return result;
    }
    if (length != sizeof(*store) || store->version != API_TOKEN_STORE_VERSION ||
        store->record_size != sizeof(StoredApiToken))
    {
        mbedtls_platform_zeroize(store, sizeof(*store));
        return ESP_ERR_INVALID_VERSION;
    }

    for (size_t index = 0; index < API_TOKEN_MAX_COUNT; index++)
    {
        if (!api_tokens_record_is_valid(&store->records[index]))
        {
            mbedtls_platform_zeroize(store, sizeof(*store));
            return ESP_ERR_INVALID_CRC;
        }
        if (store->records[index].active == 1U)
        {
            for (size_t other = index + 1U; other < API_TOKEN_MAX_COUNT; other++)
            {
                if (store->records[other].active == 1U &&
                    (api_tokens_names_equal(store->records[index].name,
                                            store->records[other].name) ||
                     api_tokens_constant_time_equal(store->records[index].id,
                                                    store->records[other].id,
                                                    API_TOKEN_ID_BYTES)))
                {
                    mbedtls_platform_zeroize(store, sizeof(*store));
                    return ESP_ERR_INVALID_CRC;
                }
            }
        }
    }
    return ESP_OK;
}

static esp_err_t api_tokens_store(const StoredApiTokenSet *store)
{
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(API_TOKEN_NAMESPACE, NVS_READWRITE, &handle);
    if (result == ESP_OK)
    {
        result = nvs_set_blob(handle, API_TOKEN_NVS_KEY, store, sizeof(*store));
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

static esp_err_t api_tokens_derive_verifier(const char *token,
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
        result = psa_hash_finish(&operation, verifier,
                                 API_TOKEN_VERIFIER_BYTES, &verifier_length);
    }
    psa_hash_abort(&operation);
    return result == PSA_SUCCESS && verifier_length == API_TOKEN_VERIFIER_BYTES
               ? ESP_OK
               : ESP_FAIL;
}

static bool api_tokens_identifier_is_zero(const uint8_t *identifier)
{
    uint8_t aggregate = 0;
    for (size_t index = 0; index < API_TOKEN_ID_BYTES; index++)
    {
        aggregate |= identifier[index];
    }
    return aggregate == 0U;
}

static bool api_tokens_identifier_is_unique(const StoredApiTokenSet *store,
                                            const uint8_t *identifier)
{
    for (size_t index = 0; index < API_TOKEN_MAX_COUNT; index++)
    {
        if (store->records[index].active == 1U &&
            api_tokens_constant_time_equal(store->records[index].id, identifier,
                                           API_TOKEN_ID_BYTES))
        {
            return false;
        }
    }
    return true;
}

static bool api_tokens_name_exists(const StoredApiTokenSet *store,
                                   const char *name)
{
    for (size_t index = 0; index < API_TOKEN_MAX_COUNT; index++)
    {
        if (store->records[index].active == 1U &&
            api_tokens_names_equal(store->records[index].name, name))
        {
            return true;
        }
    }
    return false;
}

static bool api_tokens_format_issued_at(int64_t issued_at,
                                        char destination[API_TOKEN_ISSUED_AT_LENGTH + 1U])
{
    const time_t timestamp = (time_t)issued_at;
    struct tm utc = {0};
    return gmtime_r(&timestamp, &utc) != NULL &&
           strftime(destination, API_TOKEN_ISSUED_AT_LENGTH + 1U,
                    "%Y-%m-%dT%H:%M:%SZ", &utc) == API_TOKEN_ISSUED_AT_LENGTH;
}

static bool api_tokens_metadata_from_record(const StoredApiToken *record,
                                            ApiTokenMetadata *metadata)
{
    memset(metadata, 0, sizeof(*metadata));
    api_tokens_bytes_to_hex(record->id, sizeof(record->id), metadata->id,
                            sizeof(metadata->id));
    snprintf(metadata->name, sizeof(metadata->name), "%s", record->name);
    snprintf(metadata->final_four, sizeof(metadata->final_four), "%s",
             record->final_four);
    metadata->scopes = record->scopes;
    return api_tokens_format_issued_at(record->issued_at, metadata->issued_at);
}

esp_err_t api_tokens_list(ApiTokenList *list)
{
    if (list == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(list, 0, sizeof(*list));

    StoredApiTokenSet store;
    esp_err_t result = api_tokens_load(&store);
    if (result != ESP_OK)
    {
        return result;
    }
    for (size_t index = 0; index < API_TOKEN_MAX_COUNT; index++)
    {
        if (store.records[index].active == 1U)
        {
            if (!api_tokens_metadata_from_record(&store.records[index],
                                                 &list->tokens[list->count]))
            {
                result = ESP_ERR_INVALID_STATE;
                break;
            }
            list->count++;
        }
    }
    mbedtls_platform_zeroize(&store, sizeof(store));
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(list, sizeof(*list));
    }
    return result;
}

esp_err_t api_tokens_create(const char *name, time_t issued_at, uint32_t scopes,
                            ApiTokenMetadata *metadata,
                            char token[API_TOKEN_VALUE_LENGTH + 1U])
{
    if (!api_token_name_is_valid(name) || issued_at < API_TOKEN_VALID_EPOCH ||
        scopes != API_TOKEN_SCOPE_OTA_INSTALL || metadata == NULL || token == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(metadata, 0, sizeof(*metadata));
    memset(token, 0, API_TOKEN_VALUE_LENGTH + 1U);

    StoredApiTokenSet store;
    esp_err_t result = api_tokens_load(&store);
    if (result != ESP_OK)
    {
        return result;
    }
    if (api_tokens_name_exists(&store, name))
    {
        mbedtls_platform_zeroize(&store, sizeof(store));
        return ESP_ERR_INVALID_STATE;
    }

    size_t available_index = API_TOKEN_MAX_COUNT;
    for (size_t index = 0; index < API_TOKEN_MAX_COUNT; index++)
    {
        if (store.records[index].active == 0U)
        {
            available_index = index;
            break;
        }
    }
    if (available_index == API_TOKEN_MAX_COUNT)
    {
        mbedtls_platform_zeroize(&store, sizeof(store));
        return ESP_ERR_NO_MEM;
    }

    StoredApiToken *record = &store.records[available_index];
    memset(record, 0, sizeof(*record));
    bool identifier_ready = false;
    for (size_t attempt = 0; attempt < 8U && !identifier_ready; attempt++)
    {
        esp_fill_random(record->id, sizeof(record->id));
        identifier_ready = !api_tokens_identifier_is_zero(record->id) &&
                           api_tokens_identifier_is_unique(&store, record->id);
    }
    if (!identifier_ready)
    {
        mbedtls_platform_zeroize(&store, sizeof(store));
        return ESP_FAIL;
    }

    uint8_t random_value[API_TOKEN_RANDOM_BYTES];
    esp_fill_random(random_value, sizeof(random_value));
    memcpy(token, API_TOKEN_PREFIX, sizeof(API_TOKEN_PREFIX) - 1U);
    api_tokens_bytes_to_hex(random_value, sizeof(random_value),
                            token + sizeof(API_TOKEN_PREFIX) - 1U,
                            API_TOKEN_RANDOM_BYTES * 2U + 1U);
    esp_fill_random(record->salt, sizeof(record->salt));
    result = api_tokens_derive_verifier(token, record->salt, record->verifier);
    mbedtls_platform_zeroize(random_value, sizeof(random_value));
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(token, API_TOKEN_VALUE_LENGTH + 1U);
        mbedtls_platform_zeroize(&store, sizeof(store));
        return result;
    }

    record->issued_at = (int64_t)issued_at;
    record->scopes = scopes;
    record->active = 1U;
    snprintf(record->name, sizeof(record->name), "%s", name);
    snprintf(record->final_four, sizeof(record->final_four), "%s",
             token + API_TOKEN_VALUE_LENGTH - API_TOKEN_FINAL_FOUR_LENGTH);
    if (!api_tokens_metadata_from_record(record, metadata))
    {
        result = ESP_ERR_INVALID_STATE;
    }
    else
    {
        result = api_tokens_store(&store);
    }
    mbedtls_platform_zeroize(&store, sizeof(store));
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(metadata, sizeof(*metadata));
        mbedtls_platform_zeroize(token, API_TOKEN_VALUE_LENGTH + 1U);
    }
    return result;
}

esp_err_t api_tokens_delete(const char *id)
{
    uint8_t identifier[API_TOKEN_ID_BYTES] = {0};
    if (id == NULL || strlen(id) != API_TOKEN_ID_HEX_LENGTH ||
        !api_tokens_hex_to_bytes(id, API_TOKEN_ID_HEX_LENGTH, identifier,
                                sizeof(identifier)))
    {
        return ESP_ERR_INVALID_ARG;
    }

    StoredApiTokenSet store;
    esp_err_t result = api_tokens_load(&store);
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(identifier, sizeof(identifier));
        return result;
    }

    size_t found_index = API_TOKEN_MAX_COUNT;
    for (size_t index = 0; index < API_TOKEN_MAX_COUNT; index++)
    {
        if (store.records[index].active == 1U &&
            api_tokens_constant_time_equal(store.records[index].id, identifier,
                                           sizeof(identifier)))
        {
            found_index = index;
            break;
        }
    }
    mbedtls_platform_zeroize(identifier, sizeof(identifier));
    if (found_index == API_TOKEN_MAX_COUNT)
    {
        mbedtls_platform_zeroize(&store, sizeof(store));
        return ESP_ERR_NOT_FOUND;
    }

    for (size_t index = found_index; index + 1U < API_TOKEN_MAX_COUNT; index++)
    {
        store.records[index] = store.records[index + 1U];
    }
    mbedtls_platform_zeroize(&store.records[API_TOKEN_MAX_COUNT - 1U],
                             sizeof(store.records[0]));
    result = api_tokens_store(&store);
    mbedtls_platform_zeroize(&store, sizeof(store));
    return result;
}

bool api_tokens_authorize(const char *token, uint32_t required_scope)
{
    if (!api_token_value_is_valid(token) || required_scope == 0U ||
        (required_scope & ~API_TOKEN_SCOPE_OTA_INSTALL) != 0U)
    {
        return false;
    }

    StoredApiTokenSet store;
    if (api_tokens_load(&store) != ESP_OK)
    {
        return false;
    }

    uint8_t authorized = 0U;
    uint8_t candidate[API_TOKEN_VERIFIER_BYTES] = {0};
    for (size_t index = 0; index < API_TOKEN_MAX_COUNT; index++)
    {
        const StoredApiToken *record = &store.records[index];
        if (api_tokens_derive_verifier(token, record->salt, candidate) != ESP_OK)
        {
            authorized = 0U;
            break;
        }
        const uint8_t verifier_matches =
            api_tokens_constant_time_equal(candidate, record->verifier,
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
    mbedtls_platform_zeroize(&store, sizeof(store));
    return authorized != 0U;
}
