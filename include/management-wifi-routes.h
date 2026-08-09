#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** List nearby Wi-Fi networks for an authenticated ADMIN session. */
esp_err_t management_wifi_scan_handler(httpd_req_t *request);

/** Stage new Wi-Fi credentials with the existing ADMIN and CSRF policy. */
esp_err_t management_wifi_configure_handler(httpd_req_t *request);
