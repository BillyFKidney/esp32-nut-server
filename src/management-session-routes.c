#include "management-session-routes.h"

#include <stdio.h>

#include "management-http.h"
#include "management-session.h"

esp_err_t management_session_logout_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden", "{\"error\":\"Invalid session or CSRF token.\"}");
    }
    management_session_clear();
    management_session_expire_cookie(request);
    return management_send_redirect(request, "/");
}

esp_err_t management_session_activity_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(
            request, "403 Forbidden",
            "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char response[96];
    const uint32_t remaining_seconds = management_session_remaining_seconds();
    snprintf(response, sizeof(response),
             "{\"remaining_seconds\":%u,\"warning\":%s}",
             (unsigned int)remaining_seconds,
             remaining_seconds > 0 &&
                     remaining_seconds <= MANAGEMENT_SESSION_WARNING_SECONDS
                 ? "true"
                 : "false");
    return management_send_json(request, "200 OK", response);
}
