#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Render the first-run ADMIN password setup page using the supplied CSRF value. */
esp_err_t management_pages_send_setup(httpd_req_t *request, const char *csrf);

/** Render the ADMIN sign-in page. */
esp_err_t management_pages_send_login(httpd_req_t *request);

/** Render the ADMIN login-cooldown page and its Retry-After response header. */
esp_err_t management_pages_send_login_throttled(httpd_req_t *request,
                                                int retry_after);

/** Render the authenticated ADMIN console using the supplied CSRF value. */
esp_err_t management_pages_send_admin(httpd_req_t *request, const char *csrf);
