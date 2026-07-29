#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Handle first-run ADMIN password setup. */
esp_err_t management_auth_setup_handler(httpd_req_t *request);

/** Redirect the legacy ADMIN login page path to the management root. */
esp_err_t management_auth_login_page_handler(httpd_req_t *request);

/** Handle ADMIN sign-in and failed-login cooldown behavior. */
esp_err_t management_auth_login_handler(httpd_req_t *request);

/** Handle an authenticated ADMIN password change. */
esp_err_t management_auth_password_change_handler(httpd_req_t *request);
