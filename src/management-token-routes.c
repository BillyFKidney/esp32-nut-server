/** @file management-token-routes.c @brief Handle ADMIN API-token lifecycle routes. @see management-token-routes.h, api_tokens.h, management-authorization.h, time_config.h */
#include "management-token-routes.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "api_tokens.h"
#include "management-authorization.h"
#include "management-http.h"
#include "management-session.h"
#include "time_config.h"

#include "esp_err.h"
#include "esp_log.h"
#include "mbedtls/platform_util.h"

#define TAG "nut-management"

esp_err_t management_token_list_handler(httpd_req_t *request)
{
    if (!management_require_session(request, true))
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

esp_err_t management_token_create_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
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

esp_err_t management_token_delete_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
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
