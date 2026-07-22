#pragma once

#include <stdbool.h>
#include <stdarg.h>

#include "esp_err.h"

/**
 * Start the LAN-only HTTPS administration service after station Wi-Fi has an
 * IPv4 address. The service creates a device-specific self-signed certificate
 * on first use.
 */
esp_err_t management_server_start(void);

/** Install the bounded runtime log capture used by the authenticated console. */
void management_log_capture_start(void);

/** Capture an embedded NUT syslog message without writing it to storage. */
void management_log_capture_syslog(int priority, const char *format,
                                   va_list arguments);

/** Return whether an ADMIN password has been configured. */
bool management_admin_password_is_configured(void);

/**
 * Remove administration credentials, API credentials, device identity, and
 * the device HTTPS certificate. Firmware and OTA partitions are untouched.
 */
esp_err_t management_factory_reset(void);
