/** @file wifi-provisioning-web.c @brief Serve captive-portal setup pages and APIs. @see wifi-provisioning-web.h, wifi-credentials.h, wifi-diagnostics.h, wifi-portal.h */
#include "wifi-provisioning-web.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "wifi-credentials.h"
#include "wifi-diagnostics.h"
#include "wifi-portal.h"
#include "wifi-provisioning.h"

#define WIFI_PROVISIONING_WEB_REQUEST_BODY_LIMIT 256U
#define WIFI_PROVISIONING_WEB_SCAN_RESULT_LIMIT WIFI_MANAGEMENT_SCAN_RESULT_LIMIT

static const char *TAG = "nut-wifi";

static void http_set_common_headers(httpd_req_t *request)
{
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
}

static esp_err_t http_send_json(httpd_req_t *request, const char *status,
                                const char *json)
{
    http_set_common_headers(request);
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request, json);
}

static esp_err_t portal_root_handler(httpd_req_t *request)
{
    http_set_common_headers(request);
    httpd_resp_set_hdr(request, "Content-Security-Policy",
                       "default-src 'self'; style-src 'unsafe-inline'; script-src 'unsafe-inline'; form-action 'self'");
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_send(request, wifi_portal_html, wifi_portal_html_length);
}

static esp_err_t portal_not_found_handler(httpd_req_t *request, httpd_err_code_t error)
{
    (void)error;
    http_set_common_headers(request);
    httpd_resp_set_status(request, "303 See Other");
    httpd_resp_set_hdr(request, "Location", "/");
    return httpd_resp_sendstr(request, "Continue to ESP32-NUT Wi-Fi setup");
}

static esp_err_t json_send_escaped_string(httpd_req_t *request, const char *value)
{
    if (httpd_resp_send_chunk(request, "\"", 1) != ESP_OK)
    {
        return ESP_FAIL;
    }

    for (const unsigned char *character = (const unsigned char *)value; *character != '\0'; character++)
    {
        char encoded[7];
        const char *chunk = (const char *)character;
        size_t chunk_length = 1;
        if (*character == '\\' || *character == '\"')
        {
            encoded[0] = '\\';
            encoded[1] = (char)*character;
            chunk = encoded;
            chunk_length = 2;
        }
        else if (*character < 0x20)
        {
            snprintf(encoded, sizeof(encoded), "\\u%04x", *character);
            chunk = encoded;
            chunk_length = 6;
        }

        if (httpd_resp_send_chunk(request, chunk, chunk_length) != ESP_OK)
        {
            return ESP_FAIL;
        }
    }

    return httpd_resp_send_chunk(request, "\"", 1);
}

static esp_err_t portal_networks_handler(httpd_req_t *request)
{
    WifiProvisioningWebContext *context = request->user_ctx;
    *context->connection_requested = false;
    esp_wifi_disconnect();

    esp_err_t result = esp_wifi_scan_start(NULL, true);
    if (result != ESP_OK)
    {
        ESP_LOGW(TAG, "Wi-Fi scan failed: %s", esp_err_to_name(result));
        return http_send_json(request, "503 Service Unavailable", "{\"message\":\"Wi-Fi scan failed\"}");
    }

    uint16_t access_point_count = 0;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_num(&access_point_count));
    if (access_point_count > WIFI_PROVISIONING_WEB_SCAN_RESULT_LIMIT)
    {
        access_point_count = WIFI_PROVISIONING_WEB_SCAN_RESULT_LIMIT;
    }

    wifi_ap_record_t *records = calloc(access_point_count, sizeof(*records));
    if (access_point_count > 0 && records == NULL)
    {
        return http_send_json(request, "500 Internal Server Error", "{\"message\":\"Out of memory\"}");
    }

    uint16_t records_returned = access_point_count;
    result = esp_wifi_scan_get_ap_records(&records_returned, records);
    if (result != ESP_OK)
    {
        free(records);
        return http_send_json(request, "503 Service Unavailable", "{\"message\":\"Unable to read scan results\"}");
    }

    http_set_common_headers(request);
    httpd_resp_set_type(request, "application/json");
    httpd_resp_send_chunk(request, "[", 1);
    unsigned int unique_count = 0;
    for (uint16_t index = 0; index < records_returned; index++)
    {
        char ssid[33] = {0};
        const size_t ssid_length = strnlen((const char *)records[index].ssid, sizeof(ssid) - 1);
        if (ssid_length == 0)
        {
            continue;
        }
        memcpy(ssid, records[index].ssid, ssid_length);

        bool duplicate = false;
        for (uint16_t previous = 0; previous < index; previous++)
        {
            if (strncmp((const char *)records[previous].ssid, ssid, sizeof(records[previous].ssid)) == 0)
            {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
        {
            continue;
        }

        if (unique_count++ > 0)
        {
            httpd_resp_send_chunk(request, ",", 1);
        }
        if (json_send_escaped_string(request, ssid) != ESP_OK)
        {
            free(records);
            return ESP_FAIL;
        }
    }
    free(records);
    httpd_resp_send_chunk(request, "]", 1);
    return httpd_resp_send_chunk(request, NULL, 0);
}

static esp_err_t portal_status_handler(httpd_req_t *request)
{
    char diagnostic[WIFI_CONNECTION_DIAGNOSTIC_LENGTH];
    wifi_get_connection_diagnostic(diagnostic, sizeof(diagnostic));

    http_set_common_headers(request);
    httpd_resp_set_type(request, "application/json");
    if (httpd_resp_send_chunk(request, "{\"message\":", HTTPD_RESP_USE_STRLEN) != ESP_OK ||
        json_send_escaped_string(request, diagnostic) != ESP_OK ||
        httpd_resp_send_chunk(request, "}", HTTPD_RESP_USE_STRLEN) != ESP_OK)
    {
        return ESP_FAIL;
    }
    return httpd_resp_send_chunk(request, NULL, 0);
}

static int hexadecimal_value(char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    character = (char)tolower((unsigned char)character);
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    return -1;
}

static bool url_decode(char *destination, size_t destination_size, const char *source)
{
    size_t output_length = 0;
    while (*source != '\0')
    {
        if (output_length + 1 >= destination_size)
        {
            return false;
        }

        if (*source == '+')
        {
            destination[output_length++] = ' ';
            source++;
        }
        else if (*source == '%' && source[1] != '\0' && source[2] != '\0')
        {
            const int high = hexadecimal_value(source[1]);
            const int low = hexadecimal_value(source[2]);
            if (high < 0 || low < 0)
            {
                return false;
            }
            destination[output_length++] = (char)((high << 4) | low);
            source += 3;
        }
        else
        {
            destination[output_length++] = *source++;
        }
    }
    destination[output_length] = '\0';
    return true;
}

static esp_err_t portal_configure_handler(httpd_req_t *request)
{
    WifiProvisioningWebContext *context = request->user_ctx;
    if (request->content_len <= 0 || request->content_len >= WIFI_PROVISIONING_WEB_REQUEST_BODY_LIMIT)
    {
        return http_send_json(request, "400 Bad Request", "{\"message\":\"Invalid request\"}");
    }

    char request_body[WIFI_PROVISIONING_WEB_REQUEST_BODY_LIMIT];
    size_t received = 0;
    while (received < (size_t)request->content_len)
    {
        const int result = httpd_req_recv(request, request_body + received,
                                          (size_t)request->content_len - received);
        if (result <= 0)
        {
            return http_send_json(request, "400 Bad Request", "{\"message\":\"Incomplete request\"}");
        }
        received += (size_t)result;
    }
    request_body[received] = '\0';

    char encoded_ssid[97];
    char encoded_password[190];
    WifiCredentials credentials = {0};
    if (httpd_query_key_value(request_body, "ssid", encoded_ssid, sizeof(encoded_ssid)) != ESP_OK ||
        httpd_query_key_value(request_body, "password", encoded_password, sizeof(encoded_password)) != ESP_OK ||
        !url_decode(credentials.ssid, sizeof(credentials.ssid), encoded_ssid) ||
        !url_decode(credentials.password, sizeof(credentials.password), encoded_password))
    {
        return http_send_json(request, "400 Bad Request", "{\"message\":\"Invalid network name or password\"}");
    }

    const size_t ssid_length = strlen(credentials.ssid);
    const size_t password_length = strlen(credentials.password);
    if (ssid_length == 0 || ssid_length > 32 ||
        (password_length > 0 && (password_length < 8 || password_length > 63)))
    {
        return http_send_json(request, "400 Bad Request",
                              "{\"message\":\"Use a 1-32 character network name and an 8-63 character password, or leave the password blank for an open network.\"}");
    }

    ESP_LOGI(TAG, "Saving pending credentials for Wi-Fi network '%s'", credentials.ssid);
    const esp_err_t result = wifi_pending_credentials_save(&credentials);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to save pending Wi-Fi credentials: %s", esp_err_to_name(result));
        return http_send_json(request, "500 Internal Server Error",
                              "{\"message\":\"Unable to save Wi-Fi credentials. Try again.\"}");
    }

    if (context->schedule_restart() != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to schedule Wi-Fi validation restart");
        return http_send_json(request, "500 Internal Server Error",
                              "{\"message\":\"Unable to restart for Wi-Fi validation. Try again.\"}");
    }

    return http_send_json(
        request, "200 OK",
        "{\"message\":\"Credentials saved. The device will restart and test Wi-Fi without the setup access point. They will be kept for automatic retries if the connection cannot be completed.\"}");
}

httpd_handle_t wifi_provisioning_web_start(WifiProvisioningWebContext *context)
{
    if (context == NULL || context->connection_requested == NULL ||
        context->schedule_restart == NULL)
    {
        return NULL;
    }

    httpd_config_t configuration = HTTPD_DEFAULT_CONFIG();
    configuration.max_open_sockets = 4;
    configuration.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &configuration) != ESP_OK)
    {
        return NULL;
    }

    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = portal_root_handler,
        .user_ctx = context,
    };
    const httpd_uri_t networks = {
        .uri = "/api/networks",
        .method = HTTP_GET,
        .handler = portal_networks_handler,
        .user_ctx = context,
    };
    const httpd_uri_t configure = {
        .uri = "/api/configure",
        .method = HTTP_POST,
        .handler = portal_configure_handler,
        .user_ctx = context,
    };
    const httpd_uri_t status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = portal_status_handler,
        .user_ctx = context,
    };

    if (httpd_register_uri_handler(server, &root) != ESP_OK ||
        httpd_register_uri_handler(server, &networks) != ESP_OK ||
        httpd_register_uri_handler(server, &configure) != ESP_OK ||
        httpd_register_uri_handler(server, &status) != ESP_OK ||
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, portal_not_found_handler) != ESP_OK)
    {
        httpd_stop(server);
        return NULL;
    }

    return server;
}
