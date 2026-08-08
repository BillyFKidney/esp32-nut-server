#include "management-authorization.h"
#include "management-session.h"
#include "management-http.h"
#include "api_tokens.h"

#include <string.h>
#include "mbedtls/platform_util.h"

bool management_require_session(httpd_req_t *request, bool refresh_activity)
{
    if (management_session_is_authorized(request, refresh_activity))
    {
        return true;
    }
    management_send_json(request, "401 Unauthorized",
                         "{\"error\":\"ADMIN authentication is required.\"}");
    return false;
}

bool management_require_session_without_activity(httpd_req_t *request)
{
    if (management_session_is_authorized(request, false))
    {
        return true;
    }
    management_send_json(request, "401 Unauthorized",
                         "{\"error\":\"ADMIN authentication is required.\"}");
    return false;
}

bool management_bearer_is_authorized(httpd_req_t *request,
                                     uint32_t required_scope)
{
    static const char prefix[] = "Bearer ";
    const size_t expected_length = sizeof(prefix) - 1U + API_TOKEN_VALUE_LENGTH;
    const size_t header_length =
        httpd_req_get_hdr_value_len(request, "Authorization");
    if (header_length != expected_length)
    {
        return false;
    }

    char authorization[sizeof(prefix) - 1U + API_TOKEN_VALUE_LENGTH + 1U];
    if (httpd_req_get_hdr_value_str(request, "Authorization", authorization,
                                    sizeof(authorization)) != ESP_OK)
    {
        mbedtls_platform_zeroize(authorization, sizeof(authorization));
        return false;
    }
    const bool authorized =
        strncmp(authorization, prefix, sizeof(prefix) - 1U) == 0 &&
        api_tokens_authorize(authorization + sizeof(prefix) - 1U,
                             required_scope);
    mbedtls_platform_zeroize(authorization, sizeof(authorization));
    return authorized;
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