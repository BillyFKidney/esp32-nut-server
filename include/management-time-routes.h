#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Handle ADMIN time configuration, manual time, and NTP synchronization. */
esp_err_t management_time_config_handler(httpd_req_t *request);
