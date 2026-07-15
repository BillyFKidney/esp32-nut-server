#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * Process an authenticated HTTP(S) request containing a complete ESP-IDF
 * application image. The caller owns authentication and route registration.
 */
esp_err_t ota_install_from_request(httpd_req_t *request);

/**
 * Mark a newly booted OTA image as valid after core services are running.
 */
void ota_mark_running_image_valid(void);
