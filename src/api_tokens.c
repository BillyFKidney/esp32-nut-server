/** @file api_tokens.c @brief Manage persisted API tokens and scoped bearer authorization. @see api_tokens.h, management-authorization.h, nvs.h */
#include "api_tokens.h"

#include "api-token-store.h"

#include "nvs.h"

#define API_TOKEN_NAMESPACE "management"
#define API_TOKEN_NVS_KEY "api-tokens"
#define API_TOKEN_STORE_VERSION 1U
#define DIAGNOSTIC_TOKEN_NVS_KEY "diag-tokens"
#define DIAGNOSTIC_TOKEN_STORE_VERSION 1U

typedef struct
{
    uint32_t version;
    uint32_t record_size;
    ApiTokenStoreRecord records[API_TOKEN_MAX_COUNT];
} StoredApiTokenSet;

typedef struct
{
    uint32_t version;
    uint32_t record_size;
    ApiTokenStoreRecord records[DIAGNOSTIC_TOKEN_MAX_COUNT];
} DiagnosticStoredApiTokenSet;

_Static_assert(sizeof(API_TOKEN_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "API-token NVS namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(API_TOKEN_NVS_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "API-token NVS key exceeds the ESP-IDF limit");
_Static_assert(sizeof(ApiTokenStoreRecord) == 112U,
               "API-token record layout changed; increment the store version");
_Static_assert(sizeof(StoredApiTokenSet) == 456U,
               "API-token store layout changed; increment the store version");
_Static_assert(sizeof(DiagnosticStoredApiTokenSet) == 232U,
               "Diagnostic-token store layout changed; increment the store version");

static const ApiTokenStoreConfig api_token_store_config = {
    .nvs_key = API_TOKEN_NVS_KEY,
    .version = API_TOKEN_STORE_VERSION,
    .scope = API_TOKEN_SCOPE_OTA_INSTALL,
    .record_count = API_TOKEN_MAX_COUNT,
};

static const ApiTokenStoreConfig diagnostic_token_store_config = {
    .nvs_key = DIAGNOSTIC_TOKEN_NVS_KEY,
    .version = DIAGNOSTIC_TOKEN_STORE_VERSION,
    .scope = API_TOKEN_SCOPE_DIAGNOSTICS_NUT,
    .record_count = DIAGNOSTIC_TOKEN_MAX_COUNT,
};

bool api_token_name_is_valid(const char *name)
{
    return api_token_store_name_is_valid(name);
}

esp_err_t api_tokens_list(ApiTokenList *list)
{
    if (list == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    StoredApiTokenSet store;
    return api_token_store_list(&api_token_store_config, &store, sizeof(store),
                                list->tokens, API_TOKEN_MAX_COUNT, &list->count);
}

esp_err_t api_tokens_create(const char *name, time_t issued_at, uint32_t scopes,
                            ApiTokenMetadata *metadata,
                            char token[API_TOKEN_VALUE_LENGTH + 1U])
{
    if (scopes != API_TOKEN_SCOPE_OTA_INSTALL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    StoredApiTokenSet store;
    return api_token_store_create(&api_token_store_config, &store, sizeof(store),
                                  name, issued_at, metadata, token);
}

esp_err_t api_tokens_delete(const char *id)
{
    StoredApiTokenSet store;
    return api_token_store_delete(&api_token_store_config, &store, sizeof(store),
                                  id);
}

bool api_tokens_authorize(const char *token, uint32_t required_scope)
{
    StoredApiTokenSet store;
    return api_token_store_authorize(&api_token_store_config, &store,
                                     sizeof(store), token, required_scope);
}

esp_err_t diagnostic_tokens_list(DiagnosticTokenList *list)
{
    if (list == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    DiagnosticStoredApiTokenSet store;
    return api_token_store_list(&diagnostic_token_store_config, &store,
                                sizeof(store), list->tokens,
                                DIAGNOSTIC_TOKEN_MAX_COUNT, &list->count);
}

esp_err_t diagnostic_tokens_create(const char *name, time_t issued_at,
                                   ApiTokenMetadata *metadata,
                                   char token[API_TOKEN_VALUE_LENGTH + 1U])
{
    DiagnosticStoredApiTokenSet store;
    return api_token_store_create(&diagnostic_token_store_config, &store,
                                  sizeof(store), name, issued_at, metadata, token);
}

esp_err_t diagnostic_tokens_delete(const char *id)
{
    DiagnosticStoredApiTokenSet store;
    return api_token_store_delete(&diagnostic_token_store_config, &store,
                                  sizeof(store), id);
}

bool diagnostic_tokens_authorize(const char *token)
{
    DiagnosticStoredApiTokenSet store;
    return api_token_store_authorize(&diagnostic_token_store_config, &store,
                                     sizeof(store), token,
                                     API_TOKEN_SCOPE_DIAGNOSTICS_NUT);
}
