#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "esp_http_server.h"

/** Schedule the existing reboot-and-validate Wi-Fi credential flow. */
typedef esp_err_t (*WifiProvisioningWebRestartScheduler)(void);

/**
 * Mutable state and callbacks owned by Wi-Fi provisioning orchestration.
 *
 * The web module uses this narrow context only for the existing portal routes.
 * It does not own station connection, physical recovery, AP/DNS lifecycle, or
 * credential persistence.
 */
typedef struct
{
    bool *connection_requested;
    WifiProvisioningWebRestartScheduler schedule_restart;
} WifiProvisioningWebContext;

/**
 * Start the temporary captive-portal HTTP server and register its existing
 * setup routes. Returns NULL when the server or a required route cannot start.
 */
httpd_handle_t wifi_provisioning_web_start(WifiProvisioningWebContext *context);
