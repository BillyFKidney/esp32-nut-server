/** @file management-diagnostics-routes.c @brief Serve bearer-authorized NUT diagnostic simulation routes. @see management-diagnostics-routes.h, management-authorization.h, nut-diagnostics.h */
#include "management-diagnostics-routes.h"

#include <stdlib.h>
#include <string.h>

#include "management-authorization.h"
#include "management-http.h"
#include "nut-diagnostics.h"
#include "mbedtls/platform_util.h"

static bool management_diagnostic_form_content_type_is_valid(httpd_req_t *request)
{
    static const char expected[] = "application/x-www-form-urlencoded";
    char content_type[sizeof(expected)] = {0};
    return httpd_req_get_hdr_value_len(request, "Content-Type") == sizeof(expected) - 1U &&
           httpd_req_get_hdr_value_str(request, "Content-Type", content_type,
                                      sizeof(content_type)) == ESP_OK &&
           strcmp(content_type, expected) == 0;
}

esp_err_t management_diagnostic_disconnect_start_handler(httpd_req_t *request)
{
    if (!management_diagnostic_bearer_is_authorized(request))
    {
        return management_send_diagnostic_bearer_unauthorized(request);
    }
    if (!management_diagnostic_form_content_type_is_valid(request))
    {
        return management_send_json(request, "415 Unsupported Media Type",
                                    "{\"error\":\"Disconnect simulation requires an application/x-www-form-urlencoded body.\"}");
    }
    char body[MANAGEMENT_FORM_BODY_LIMIT + 1U] = {0};
    char duration_text[4] = {0};
    const bool valid_form =
        management_read_form_body(request, body, sizeof(body)) == ESP_OK &&
        management_form_value(body, "duration_seconds", duration_text,
                              sizeof(duration_text));
    char *end = NULL;
    const unsigned long duration = valid_form
                                       ? strtoul(duration_text, &end, 10)
                                       : 0UL;
    const bool valid_duration = valid_form && end != duration_text && *end == '\0' &&
                                duration <= UINT32_MAX &&
                                nut_diagnostics_start_disconnect_simulation((uint32_t)duration);
    mbedtls_platform_zeroize(body, sizeof(body));
    mbedtls_platform_zeroize(duration_text, sizeof(duration_text));
    if (!valid_duration)
    {
        return management_send_json(request, "400 Bad Request",
                                    "{\"error\":\"duration_seconds must be an integer from 1 through 300.\"}");
    }
    return management_send_json(request, "200 OK",
                                "{\"message\":\"NUT disconnect simulation active.\"}");
}

esp_err_t management_diagnostic_disconnect_clear_handler(httpd_req_t *request)
{
    if (!management_diagnostic_bearer_is_authorized(request))
    {
        return management_send_diagnostic_bearer_unauthorized(request);
    }
    if (request->content_len != 0)
    {
        return management_send_json(request, "400 Bad Request",
                                    "{\"error\":\"Disconnect simulation clear does not accept a request body.\"}");
    }
    nut_diagnostics_clear_disconnect_simulation();
    return management_send_json(request, "200 OK",
                                "{\"message\":\"NUT disconnect simulation cleared.\"}");
}
