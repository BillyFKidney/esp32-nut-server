/** @file management-ota-routes.c @brief Authorize and dispatch management OTA routes. @see management-ota-routes.h, management-authorization.h, api_tokens.h, ota.h */
#include "management-ota-routes.h"

#include <string.h>

#include "api_tokens.h"
#include "management-authorization.h"
#include "management-http.h"
#include "management-session.h"
#include "ota.h"

static esp_err_t management_ota_require_session_csrf(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden", "{\"error\":\"Invalid session or CSRF token.\"}");
    }
    return ESP_OK;
}

static esp_err_t management_ota_check_content_type(httpd_req_t *request)
{
    static const char expected_content_type[] = "application/octet-stream";
    char content_type[sizeof(expected_content_type)] = {0};
    if (httpd_req_get_hdr_value_len(request, "Content-Type") != sizeof(expected_content_type) - 1U ||
        httpd_req_get_hdr_value_str(request, "Content-Type", content_type,
                                    sizeof(content_type)) != ESP_OK ||
        strcmp(content_type, expected_content_type) != 0)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t management_ota_reject_content_type(httpd_req_t *request, const char *message)
{
    return management_send_json(request, "415 Unsupported Media Type", message);
}

esp_err_t management_ota_install_handler(httpd_req_t *request)
{
    esp_err_t result = management_ota_require_session_csrf(request);
    if (result != ESP_OK)
    {
        return result;
    }
    return ota_install_from_request(request);
}

esp_err_t management_ota_check_handler(httpd_req_t *request)
{
    esp_err_t result = management_ota_require_session_csrf(request);
    if (result != ESP_OK)
    {
        return result;
    }

    result = management_ota_check_content_type(request);
    if (result != ESP_OK)
    {
        return management_ota_reject_content_type(request, "{\"error\":\"Firmware check requires an application/octet-stream image body.\"}");
    }
    return ota_check_from_request(request);
}

esp_err_t management_agent_ota_install_handler(httpd_req_t *request)
{
    if (!management_bearer_is_authorized(request, API_TOKEN_SCOPE_OTA_INSTALL))
    {
        return management_send_bearer_unauthorized(request);
    }

    esp_err_t result = management_ota_check_content_type(request);
    if (result != ESP_OK)
    {
        return management_ota_reject_content_type(request, "{\"error\":\"Agent OTA requires an application/octet-stream firmware body.\"}");
    }
    return ota_install_from_request(request);
}
