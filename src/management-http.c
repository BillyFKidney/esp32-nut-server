/** @file management-http.c @brief Send bounded management HTTP responses and parse forms. @see management-http.h, esp_http_server.h, mbedtls/platform_util.h */
#include "management-http.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "mbedtls/platform_util.h"

static void management_set_response_headers(httpd_req_t *request)
{
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "Pragma", "no-cache");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(request, "X-Frame-Options", "DENY");
    httpd_resp_set_hdr(request, "Referrer-Policy", "no-referrer");
}

esp_err_t management_send_html(httpd_req_t *request, const char *html)
{
    management_set_response_headers(request);
    httpd_resp_set_hdr(request, "Content-Security-Policy",
                       "default-src 'self'; style-src 'unsafe-inline'; script-src 'unsafe-inline'; base-uri 'none'; frame-ancestors 'none'; form-action 'self'");
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_sendstr(request, html);
}

esp_err_t management_send_html_status(httpd_req_t *request, const char *status,
                                      const char *html)
{
    httpd_resp_set_status(request, status);
    return management_send_html(request, html);
}

esp_err_t management_send_json(httpd_req_t *request, const char *status,
                               const char *json)
{
    management_set_response_headers(request);
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request, json);
}

esp_err_t management_send_redirect(httpd_req_t *request, const char *location)
{
    management_set_response_headers(request);
    httpd_resp_set_status(request, "303 See Other");
    httpd_resp_set_hdr(request, "Location", location);
    return httpd_resp_sendstr(request, "Continue");
}

bool management_json_append(char *destination, size_t destination_size,
                            size_t *used, const char *format, ...)
{
    if (destination == NULL || used == NULL || *used >= destination_size)
    {
        return false;
    }

    va_list arguments;
    va_start(arguments, format);
    const int written = vsnprintf(destination + *used,
                                  destination_size - *used, format, arguments);
    va_end(arguments);
    if (written < 0 || (size_t)written >= destination_size - *used)
    {
        *used = destination_size;
        return false;
    }
    *used += (size_t)written;
    return true;
}

bool management_json_append_string(char *destination, size_t destination_size,
                                   size_t *used, const char *value)
{
    if (!management_json_append(destination, destination_size, used, "\""))
    {
        return false;
    }

    if (value == NULL)
    {
        value = "";
    }
    for (const unsigned char *cursor = (const unsigned char *)value; *cursor != '\0'; cursor++)
    {
        switch (*cursor)
        {
        case '\\':
            if (!management_json_append(destination, destination_size, used, "\\\\"))
            {
                return false;
            }
            break;
        case '"':
            if (!management_json_append(destination, destination_size, used, "\\\""))
            {
                return false;
            }
            break;
        case '\b':
            if (!management_json_append(destination, destination_size, used, "\\b"))
            {
                return false;
            }
            break;
        case '\f':
            if (!management_json_append(destination, destination_size, used, "\\f"))
            {
                return false;
            }
            break;
        case '\n':
            if (!management_json_append(destination, destination_size, used, "\\n"))
            {
                return false;
            }
            break;
        case '\r':
            if (!management_json_append(destination, destination_size, used, "\\r"))
            {
                return false;
            }
            break;
        case '\t':
            if (!management_json_append(destination, destination_size, used, "\\t"))
            {
                return false;
            }
            break;
        default:
            if (*cursor < 0x20U &&
                !management_json_append(destination, destination_size, used,
                                        "\\u%04x", (unsigned int)*cursor))
            {
                return false;
            }
            else if (*cursor >= 0x20U &&
                     !management_json_append(destination, destination_size, used,
                                             "%c", (char)*cursor))
            {
                return false;
            }
            break;
        }
    }
    return management_json_append(destination, destination_size, used, "\"");
}

static bool management_extract_form_value(char *body, const char *expected_name,
                                          char *destination, size_t destination_size)
{
    char *save_pointer = NULL;
    for (char *pair = strtok_r(body, "&", &save_pointer); pair != NULL;
         pair = strtok_r(NULL, "&", &save_pointer))
    {
        char *separator = strchr(pair, '=');
        if (separator == NULL)
        {
            continue;
        }
        *separator = '\0';
        char *encoded_value = separator + 1;
        char decoded_name[40];
        size_t name_length = 0;
        size_t name_index = 0;
        for (; pair[name_index] != '\0' && name_length + 1 < sizeof(decoded_name); name_index++)
        {
            if (pair[name_index] == '%' && pair[name_index + 1] != '\0' && pair[name_index + 2] != '\0')
            {
                char hexadecimal[3] = {pair[name_index + 1], pair[name_index + 2], '\0'};
                decoded_name[name_length++] = (char)strtol(hexadecimal, NULL, 16);
                name_index += 2;
            }
            else
            {
                decoded_name[name_length++] = pair[name_index] == '+' ? ' ' : pair[name_index];
            }
        }
        decoded_name[name_length] = '\0';
        if (pair[name_index] != '\0' || strcmp(decoded_name, expected_name) != 0)
        {
            continue;
        }

        size_t value_length = 0;
        size_t value_index = 0;
        for (; encoded_value[value_index] != '\0' && value_length + 1 < destination_size; value_index++)
        {
            if (encoded_value[value_index] == '%' && encoded_value[value_index + 1] != '\0' &&
                encoded_value[value_index + 2] != '\0')
            {
                char hexadecimal[3] = {encoded_value[value_index + 1], encoded_value[value_index + 2], '\0'};
                destination[value_length++] = (char)strtol(hexadecimal, NULL, 16);
                value_index += 2;
            }
            else
            {
                destination[value_length++] = encoded_value[value_index] == '+' ? ' ' : encoded_value[value_index];
            }
        }
        destination[value_length] = '\0';
        return encoded_value[value_index] == '\0';
    }
    return false;
}

esp_err_t management_read_form_body(httpd_req_t *request, char *body,
                                    size_t body_size)
{
    if (request->content_len <= 0 || request->content_len > MANAGEMENT_FORM_BODY_LIMIT ||
        (size_t)request->content_len >= body_size)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    int received_total = 0;
    while (received_total < request->content_len)
    {
        const int received = httpd_req_recv(request, body + received_total,
                                            request->content_len - received_total);
        if (received <= 0)
        {
            return received == HTTPD_SOCK_ERR_TIMEOUT ? ESP_ERR_TIMEOUT : ESP_FAIL;
        }
        received_total += received;
    }
    body[received_total] = '\0';
    return ESP_OK;
}

bool management_form_value(const char *body, const char *name,
                           char *destination, size_t destination_size)
{
    char body_copy[MANAGEMENT_FORM_BODY_LIMIT + 1];
    snprintf(body_copy, sizeof(body_copy), "%s", body);
    const bool found = management_extract_form_value(body_copy, name, destination,
                                                      destination_size);
    mbedtls_platform_zeroize(body_copy, sizeof(body_copy));
    return found;
}
