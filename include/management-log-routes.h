#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Handle the ADMIN-only full retained log response without session activity refresh. */
esp_err_t management_logs_handler(httpd_req_t *request);
