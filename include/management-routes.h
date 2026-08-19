#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#define MANAGEMENT_HTTPS_ROUTE_CAPACITY 23U

/** Register the fixed HTTPS management route inventory in its existing order. */
esp_err_t management_routes_register(
    httpd_handle_t server,
    esp_err_t (*root_handler)(httpd_req_t *request));
