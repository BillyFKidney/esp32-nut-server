#pragma once

#include "esp_err.h"

/**
 * Start the development OTA HTTP server on the connected station network.
 *
 * The server listens on TCP port 8080. POST a complete ESP-IDF application
 * image to /ota to install it into the inactive OTA partition.
 */
esp_err_t ota_server_start(void);

/**
 * Mark a newly booted OTA image as valid after core services are running.
 */
void ota_mark_running_image_valid(void);
