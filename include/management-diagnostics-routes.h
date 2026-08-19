#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Start a bounded bearer-authorized NUT disconnect simulation. */
esp_err_t management_diagnostic_disconnect_start_handler(httpd_req_t *request);

/** Clear a bearer-authorized NUT disconnect simulation. */
esp_err_t management_diagnostic_disconnect_clear_handler(httpd_req_t *request);
