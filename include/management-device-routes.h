#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Handle the ADMIN device identity and log-level save route. */
esp_err_t management_device_config_handler(httpd_req_t *request);
