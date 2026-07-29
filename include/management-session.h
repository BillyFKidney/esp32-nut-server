#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_http_server.h"

#define MANAGEMENT_SESSION_HEX_LENGTH 64U
#define MANAGEMENT_SESSION_IDLE_SECONDS (15U * 60U)
#define MANAGEMENT_SESSION_WARNING_SECONDS (5U * 60U)
#define MANAGEMENT_LOGIN_COOLDOWN_SECONDS 60U

/** Start a new ADMIN session with fresh cookie and CSRF values. */
void management_session_start(void);

/** Add the current ADMIN session cookie to an HTTPS response. */
void management_session_set_cookie(httpd_req_t *request, char *session_header,
                                   size_t session_header_size);

/** Expire the ADMIN session cookie in an HTTPS response. */
void management_session_expire_cookie(httpd_req_t *request);

/** Create or reuse the temporary first-run setup cookie and its CSRF value. */
void management_session_start_setup(httpd_req_t *request, char *csrf,
                                    size_t csrf_size, char *setup_header,
                                    size_t setup_header_size);

/** Validate the temporary first-run setup cookie and CSRF value. */
bool management_session_setup_csrf_is_valid(httpd_req_t *request,
                                            const char *csrf);

/** Return the remaining ADMIN session duration in whole seconds. */
uint32_t management_session_remaining_seconds(void);

/** Copy the current ADMIN CSRF value into a caller-provided buffer. */
void management_session_copy_csrf(char *csrf, size_t csrf_size);

/** Check an ADMIN session cookie and optionally refresh its idle timer. */
bool management_session_is_authorized(httpd_req_t *request,
                                      bool refresh_activity);

/** Validate the ADMIN session cookie and X-ESP32-NUT-CSRF request header. */
bool management_session_csrf_is_valid(httpd_req_t *request);

/** Return login cooldown time remaining for the supplied monotonic time. */
int management_session_login_retry_after_seconds(int64_t now);

/** Record a failed login and report whether it began the cooldown period. */
bool management_session_record_login_failure(int64_t now);

/** Clear login-throttling state after a successful login or factory reset. */
void management_session_record_login_success(void);

/** Zeroize the in-memory ADMIN session. */
void management_session_clear(void);
