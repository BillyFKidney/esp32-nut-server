#include "ota.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "nut-ota"
#define OTA_HTTP_PORT 8080
#define OTA_RECEIVE_BUFFER_SIZE 4096
#define OTA_REBOOT_DELAY_MS 1000

static httpd_handle_t ota_http_server;
static bool ota_update_in_progress;

static void ota_set_response_headers(httpd_req_t *request)
{
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
}

static esp_err_t ota_send_error(httpd_req_t *request, const char *status,
                                const char *message)
{
    ota_set_response_headers(request);
    httpd_resp_set_status(request, status);
    return httpd_resp_sendstr(request, message);
}

static void ota_reboot_task(void *parameter)
{
    (void)parameter;
    vTaskDelay(pdMS_TO_TICKS(OTA_REBOOT_DELAY_MS));
    ESP_LOGI(TAG, "Restarting into the OTA image");
    esp_restart();
}

static esp_err_t ota_status_handler(httpd_req_t *request)
{
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    const esp_partition_t *next_partition = esp_ota_get_next_update_partition(NULL);

    ota_set_response_headers(request);
    char response[256];
    snprintf(response, sizeof(response),
             "{\"status\":\"ready\",\"port\":%d,\"running_partition\":\"%s\","
             "\"next_partition\":\"%s\",\"max_image_bytes\":%lu}",
             OTA_HTTP_PORT,
             running_partition != NULL ? running_partition->label : "unknown",
             next_partition != NULL ? next_partition->label : "unknown",
             next_partition != NULL ? (unsigned long)next_partition->size : 0UL);
    return httpd_resp_sendstr(request, response);
}

static esp_err_t ota_upload_handler(httpd_req_t *request)
{
    if (ota_update_in_progress)
    {
        return ota_send_error(request, "409 Conflict",
                              "{\"status\":\"busy\",\"message\":\"An OTA update is already in progress.\"}");
    }

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL)
    {
        return ota_send_error(request, "500 Internal Server Error",
                              "{\"status\":\"error\",\"message\":\"No inactive OTA partition is available.\"}");
    }

    if (request->content_len <= 0 || (size_t)request->content_len > update_partition->size)
    {
        return ota_send_error(request, "413 Payload Too Large",
                              "{\"status\":\"error\",\"message\":\"The uploaded image does not fit in the inactive OTA partition.\"}");
    }

    ota_update_in_progress = true;
    ESP_LOGI(TAG, "Receiving %d-byte OTA image for partition '%s'", request->content_len,
             update_partition->label);

    esp_ota_handle_t update_handle = 0;
    esp_err_t result = esp_ota_begin(update_partition, request->content_len, &update_handle);
    if (result != ESP_OK)
    {
        ota_update_in_progress = false;
        ESP_LOGE(TAG, "Unable to begin OTA update: %s", esp_err_to_name(result));
        return ota_send_error(request, "500 Internal Server Error",
                              "{\"status\":\"error\",\"message\":\"Unable to begin OTA update.\"}");
    }

    char receive_buffer[OTA_RECEIVE_BUFFER_SIZE];
    int remaining = request->content_len;
    while (remaining > 0)
    {
        const size_t receive_size = remaining < (int)sizeof(receive_buffer)
                                        ? (size_t)remaining
                                        : sizeof(receive_buffer);
        const int received = httpd_req_recv(request, receive_buffer, receive_size);
        if (received <= 0)
        {
            result = received == HTTPD_SOCK_ERR_TIMEOUT ? ESP_ERR_TIMEOUT : ESP_FAIL;
            ESP_LOGE(TAG, "OTA image receive failed: %s", esp_err_to_name(result));
            break;
        }

        result = esp_ota_write(update_handle, receive_buffer, received);
        if (result != ESP_OK)
        {
            ESP_LOGE(TAG, "OTA image write failed: %s", esp_err_to_name(result));
            break;
        }
        remaining -= received;
    }

    if (result == ESP_OK)
    {
        result = esp_ota_end(update_handle);
        if (result != ESP_OK)
        {
            ESP_LOGE(TAG, "OTA image verification failed: %s", esp_err_to_name(result));
        }
    }
    else
    {
        esp_ota_abort(update_handle);
    }

    if (result == ESP_OK)
    {
        result = esp_ota_set_boot_partition(update_partition);
        if (result != ESP_OK)
        {
            ESP_LOGE(TAG, "Unable to select OTA partition for boot: %s", esp_err_to_name(result));
        }
    }

    ota_update_in_progress = false;
    if (result != ESP_OK)
    {
        return ota_send_error(request, "422 Unprocessable Content",
                              "{\"status\":\"error\",\"message\":\"The uploaded file is not a valid ESP32-NUT firmware image.\"}");
    }

    ESP_LOGI(TAG, "OTA image verified and selected for the next boot");
    ota_set_response_headers(request);
    const esp_err_t response_result = httpd_resp_sendstr(
        request, "{\"status\":\"installed\",\"message\":\"Firmware verified. Restarting now.\"}");
    if (response_result == ESP_OK &&
        xTaskCreate(ota_reboot_task, "ota-reboot", 2048, NULL, 5, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Unable to schedule OTA reboot");
    }
    return response_result;
}

esp_err_t ota_server_start(void)
{
    if (ota_http_server != NULL)
    {
        return ESP_OK;
    }

    httpd_config_t configuration = HTTPD_DEFAULT_CONFIG();
    configuration.server_port = OTA_HTTP_PORT;
    configuration.ctrl_port = OTA_HTTP_PORT + 1;
    configuration.stack_size = 8192;
    configuration.max_open_sockets = 2;
    configuration.recv_wait_timeout = 30;
    configuration.send_wait_timeout = 30;
    configuration.lru_purge_enable = true;

    esp_err_t result = httpd_start(&ota_http_server, &configuration);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to start development OTA server: %s", esp_err_to_name(result));
        ota_http_server = NULL;
        return result;
    }

    const httpd_uri_t status = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = ota_status_handler,
    };
    const httpd_uri_t upload = {
        .uri = "/ota",
        .method = HTTP_POST,
        .handler = ota_upload_handler,
    };
    result = httpd_register_uri_handler(ota_http_server, &status);
    if (result == ESP_OK)
    {
        result = httpd_register_uri_handler(ota_http_server, &upload);
    }
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to register OTA HTTP handlers: %s", esp_err_to_name(result));
        httpd_stop(ota_http_server);
        ota_http_server = NULL;
        return result;
    }

    ESP_LOGW(TAG, "Development OTA server is listening on TCP port %d without authentication", OTA_HTTP_PORT);
    return ESP_OK;
}

void ota_mark_running_image_valid(void)
{
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (running_partition != NULL &&
        esp_ota_get_state_partition(running_partition, &state) == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        const esp_err_t result = esp_ota_mark_app_valid_cancel_rollback();
        if (result == ESP_OK)
        {
            ESP_LOGI(TAG, "New OTA image marked valid");
        }
        else
        {
            ESP_LOGE(TAG, "Unable to mark OTA image valid: %s", esp_err_to_name(result));
        }
    }
#endif
}
