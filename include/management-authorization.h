#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_http_server.h"

/**
 * Check an ADMIN session cookie and optionally refresh its idle timer.
 *
 * @param request The HTTP request to check
 * @param refresh_activity Whether to refresh the session activity timer
 * @return true if the session is valid, false otherwise
 */
bool management_require_session(httpd_req_t *request, bool refresh_activity);

/**
 * Check an ADMIN session cookie without refreshing its idle timer.
 *
 * @param request The HTTP request to check
 * @return true if the session is valid, false otherwise
 */
bool management_require_session_without_activity(httpd_req_t *request);

/**
 * Verify a Bearer token for the requested scope.
 *
 * @param request The HTTP request containing the Authorization header
 * @param required_scope The API token scope required
 * @return true if the token is valid and has the required scope, false otherwise
 */
bool management_bearer_is_authorized(httpd_req_t *request, uint32_t required_scope);

/** Verify a Bearer token from the separately persisted diagnostics.nut store. */
bool management_diagnostic_bearer_is_authorized(httpd_req_t *request);

/**
 * Send a 401 Unauthorized response with proper WWW-Authenticate header for bearer tokens.
 *
 * @param request The HTTP request to send the response to
 * @return ESP_OK on success, error code on failure
 */
esp_err_t management_send_bearer_unauthorized(httpd_req_t *request);

/** Send the diagnostics.nut bearer challenge without changing OTA responses. */
esp_err_t management_send_diagnostic_bearer_unauthorized(httpd_req_t *request);
