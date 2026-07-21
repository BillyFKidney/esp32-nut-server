#pragma once

#include <stddef.h>

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * Process an authenticated HTTP(S) request containing a complete ESP-IDF
 * application image. The caller owns authentication and route registration.
 */
esp_err_t ota_install_from_request(httpd_req_t *request);

/**
 * Copy the most recent non-secret OTA result into the caller's buffer.
 * The default is "not_available" when no update has been attempted.
 */
esp_err_t ota_get_last_result(char *destination, size_t destination_size);

/**
 * Mark a newly booted OTA image as valid after core services are running.
 */
void ota_mark_running_image_valid(void);
