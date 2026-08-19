#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** List active API-token metadata. */
esp_err_t management_token_list_handler(httpd_req_t *request);

/** Create an API token with the existing ADMIN and CSRF policy. */
esp_err_t management_token_create_handler(httpd_req_t *request);

/** Delete an API token with the existing ADMIN and CSRF policy. */
esp_err_t management_token_delete_handler(httpd_req_t *request);

/** List diagnostics.nut token metadata. */
esp_err_t management_diagnostic_token_list_handler(httpd_req_t *request);

/** Create a diagnostics.nut token with ADMIN and CSRF protection. */
esp_err_t management_diagnostic_token_create_handler(httpd_req_t *request);

/** Delete a diagnostics.nut token with ADMIN and CSRF protection. */
esp_err_t management_diagnostic_token_delete_handler(httpd_req_t *request);
