/** @file management-authorization.c @brief Enforce shared session and bearer authorization boundaries. @see management-authorization.h, management-session.h, api_tokens.h, management-http.h */
#include "management-authorization.h"
#include "management-session.h"
#include "management-http.h"
#include "api_tokens.h"

#include <string.h>
#include "mbedtls/platform_util.h"

static const char MANAGEMENT_AUTH_BEARER_PREFIX[] = "Bearer ";

typedef bool (*management_bearer_authorize_fn)(const char *token, uint32_t required_scope);

static bool management_require_session_impl(httpd_req_t *request, bool refresh_activity)
{
    if (management_session_is_authorized(request, refresh_activity))
    {
        return true;
    }
    management_send_json(request, "401 Unauthorized",
                         "{\"error\":\"ADMIN authentication is required.\"}");
    return false;
}

bool management_require_session(httpd_req_t *request, bool refresh_activity)
{
    return management_require_session_impl(request, refresh_activity);
}

bool management_require_session_without_activity(httpd_req_t *request)
{
    return management_require_session_impl(request, false);
}

static bool management_bearer_check(httpd_req_t *request,
                                    management_bearer_authorize_fn authorize_fn,
                                    uint32_t required_scope)
{
    const size_t expected_length = sizeof(MANAGEMENT_AUTH_BEARER_PREFIX) - 1U + API_TOKEN_VALUE_LENGTH;
    if (httpd_req_get_hdr_value_len(request, "Authorization") != expected_length)
    {
        return false;
    }
    char authorization[sizeof(MANAGEMENT_AUTH_BEARER_PREFIX) - 1U + API_TOKEN_VALUE_LENGTH + 1U];
    if (httpd_req_get_hdr_value_str(request, "Authorization", authorization,
                                    sizeof(authorization)) != ESP_OK)
    {
        mbedtls_platform_zeroize(authorization, sizeof(authorization));
        return false;
    }
    const bool authorized =
        strncmp(authorization, MANAGEMENT_AUTH_BEARER_PREFIX, sizeof(MANAGEMENT_AUTH_BEARER_PREFIX) - 1U) == 0 &&
        authorize_fn(authorization + sizeof(MANAGEMENT_AUTH_BEARER_PREFIX) - 1U, required_scope);
    mbedtls_platform_zeroize(authorization, sizeof(authorization));
    return authorized;
}

static bool management_bearer_authorize_ota(const char *token, uint32_t required_scope)
{
    return api_tokens_authorize(token, required_scope);
}

static bool management_bearer_authorize_diagnostic(const char *token, uint32_t required_scope)
{
    (void)required_scope;
    return diagnostic_tokens_authorize(token);
}

bool management_bearer_is_authorized(httpd_req_t *request, uint32_t required_scope)
{
    return management_bearer_check(request, management_bearer_authorize_ota, required_scope);
}

bool management_diagnostic_bearer_is_authorized(httpd_req_t *request)
{
    return management_bearer_check(request, management_bearer_authorize_diagnostic, 0U);
}

esp_err_t management_send_bearer_unauthorized(httpd_req_t *request)
{
    httpd_resp_set_hdr(
        request, "WWW-Authenticate",
        "Bearer realm=\"ESP32-NUT Agent OTA\", scope=\"ota.install\"");
    return management_send_json(
        request, "401 Unauthorized",
        "{\"error\":\"A valid API token with ota.install scope is required.\"}");
}

esp_err_t management_send_diagnostic_bearer_unauthorized(httpd_req_t *request)
{
    httpd_resp_set_hdr(
        request, "WWW-Authenticate",
        "Bearer realm=\"ESP32-NUT Diagnostics\", scope=\"diagnostics.nut\"");
    return management_send_json(
        request, "401 Unauthorized",
        "{\"error\":\"A valid API token with diagnostics.nut scope is required.\"}");
}
