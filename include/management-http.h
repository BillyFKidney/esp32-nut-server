#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_http_server.h"

#define MANAGEMENT_FORM_BODY_LIMIT 640

/**
 * Send an HTML response with the management service's existing defensive
 * response headers and Content Security Policy.
 */
esp_err_t management_send_html(httpd_req_t *request, const char *html);

/** Send an HTML response with an explicit HTTP status. */
esp_err_t management_send_html_status(httpd_req_t *request, const char *status,
                                      const char *html);

/** Send a JSON response with the management service's defensive headers. */
esp_err_t management_send_json(httpd_req_t *request, const char *status,
                               const char *json);

/** Send the management service's existing POST/redirect/get response. */
esp_err_t management_send_redirect(httpd_req_t *request, const char *location);

/** Safely append formatted JSON content to a bounded response buffer. */
bool management_json_append(char *destination, size_t destination_size,
                            size_t *used, const char *format, ...);

/** Safely append one JSON-escaped string to a bounded response buffer. */
bool management_json_append_string(char *destination, size_t destination_size,
                                   size_t *used, const char *value);

/** Read one bounded application/x-www-form-urlencoded request body. */
esp_err_t management_read_form_body(httpd_req_t *request, char *body,
                                    size_t body_size);

/** Decode one named application/x-www-form-urlencoded field. */
bool management_form_value(const char *body, const char *name,
                           char *destination, size_t destination_size);
