#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Install a browser-uploaded firmware image with ADMIN and CSRF protection. */
esp_err_t management_ota_install_handler(httpd_req_t *request);

/** Check a browser-uploaded firmware image with ADMIN and CSRF protection. */
esp_err_t management_ota_check_handler(httpd_req_t *request);

/** Install an Agent-uploaded firmware image with the ota.install bearer scope. */
esp_err_t management_agent_ota_install_handler(httpd_req_t *request);
