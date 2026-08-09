#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Handle ADMIN logout with the existing session and CSRF checks. */
esp_err_t management_session_logout_handler(httpd_req_t *request);

/** Report the remaining ADMIN session time after the existing CSRF check. */
esp_err_t management_session_activity_handler(httpd_req_t *request);
