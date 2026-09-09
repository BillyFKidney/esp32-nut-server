/** @file api-token-store.h @brief Private fixed-capacity API-token persistence helpers. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "api_tokens.h"

typedef struct
{
    int64_t issued_at;
    uint32_t scopes;
    uint8_t active;
    uint8_t reserved_before_id[3];
    uint8_t id[API_TOKEN_ID_BYTES];
    uint8_t salt[16U];
    uint8_t verifier[32U];
    char name[API_TOKEN_NAME_MAX_LENGTH + 1U];
    char final_four[API_TOKEN_FINAL_FOUR_LENGTH + 1U];
    uint8_t reserved_after_final_four[2];
} ApiTokenStoreRecord;

typedef struct
{
    const char *nvs_key;
    uint32_t version;
    uint32_t scope;
    size_t record_count;
} ApiTokenStoreConfig;

bool api_token_store_name_is_valid(const char *name);

esp_err_t api_token_store_list(const ApiTokenStoreConfig *config, void *storage,
                               size_t storage_size, ApiTokenMetadata *metadata,
                               size_t metadata_capacity, size_t *metadata_count);

esp_err_t api_token_store_create(const ApiTokenStoreConfig *config, void *storage,
                                 size_t storage_size, const char *name,
                                 time_t issued_at, ApiTokenMetadata *metadata,
                                 char token[API_TOKEN_VALUE_LENGTH + 1U]);

esp_err_t api_token_store_delete(const ApiTokenStoreConfig *config, void *storage,
                                 size_t storage_size, const char *id);

bool api_token_store_authorize(const ApiTokenStoreConfig *config, void *storage,
                               size_t storage_size, const char *token,
                               uint32_t required_scope);
