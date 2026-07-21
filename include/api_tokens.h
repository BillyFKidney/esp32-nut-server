#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "esp_err.h"

#define API_TOKEN_MAX_COUNT 4U
#define API_TOKEN_NAME_MAX_LENGTH 32U
#define API_TOKEN_RANDOM_BYTES 32U
#define API_TOKEN_PREFIX "esp32nut_v1_"
#define API_TOKEN_VALUE_LENGTH \
    ((sizeof(API_TOKEN_PREFIX) - 1U) + (API_TOKEN_RANDOM_BYTES * 2U))
#define API_TOKEN_ID_BYTES 8U
#define API_TOKEN_ID_HEX_LENGTH (API_TOKEN_ID_BYTES * 2U)
#define API_TOKEN_FINAL_FOUR_LENGTH 4U
#define API_TOKEN_ISSUED_AT_LENGTH 20U

#define API_TOKEN_SCOPE_OTA_INSTALL (1U << 0)

typedef struct
{
    char id[API_TOKEN_ID_HEX_LENGTH + 1U];
    char name[API_TOKEN_NAME_MAX_LENGTH + 1U];
    char issued_at[API_TOKEN_ISSUED_AT_LENGTH + 1U];
    char final_four[API_TOKEN_FINAL_FOUR_LENGTH + 1U];
    uint32_t scopes;
} ApiTokenMetadata;

typedef struct
{
    size_t count;
    ApiTokenMetadata tokens[API_TOKEN_MAX_COUNT];
} ApiTokenList;

/** Return whether a token name is safe, non-empty, and within the fixed limit. */
bool api_token_name_is_valid(const char *name);

/**
 * Create and persist a named token. The caller receives the plaintext token
 * exactly once and must zeroize it after sending the creation response.
 */
esp_err_t api_tokens_create(const char *name, time_t issued_at, uint32_t scopes,
                            ApiTokenMetadata *metadata,
                            char token[API_TOKEN_VALUE_LENGTH + 1U]);

/** List only non-secret metadata for active tokens. */
esp_err_t api_tokens_list(ApiTokenList *list);

/** Delete an active token by its public random identifier. */
esp_err_t api_tokens_delete(const char *id);

/** Verify a complete token for the requested scope. */
bool api_tokens_authorize(const char *token, uint32_t required_scope);
