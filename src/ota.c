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
#include "nvs.h"

#define TAG "nut-ota"
#define OTA_NVS_NAMESPACE "management"
#define OTA_LAST_RESULT_KEY "ota-result"
#define OTA_RECEIVE_BUFFER_SIZE 4096
#define OTA_RECEIVE_TIMEOUT_RETRIES 3
#define OTA_REBOOT_DELAY_MS 1000

static bool ota_update_in_progress;

static void ota_record_result(const char *result)
{
    nvs_handle_t handle = 0;
    esp_err_t nvs_result = nvs_open(OTA_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (nvs_result == ESP_OK)
    {
        nvs_result = nvs_set_str(handle, OTA_LAST_RESULT_KEY, result);
    }
    if (nvs_result == ESP_OK)
    {
        nvs_result = nvs_commit(handle);
    }
    if (handle != 0)
    {
        nvs_close(handle);
    }
    if (nvs_result != ESP_OK)
    {
        ESP_LOGW(TAG, "Unable to record OTA result: %s", esp_err_to_name(nvs_result));
    }
}

esp_err_t ota_get_last_result(char *destination, size_t destination_size)
{
    if (destination == NULL || destination_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(destination, destination_size, "not_available");
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(OTA_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        return ESP_OK;
    }
    if (result != ESP_OK)
    {
        snprintf(destination, destination_size, "unavailable");
        return result;
    }

    size_t stored_length = destination_size;
    result = nvs_get_str(handle, OTA_LAST_RESULT_KEY, destination, &stored_length);
    nvs_close(handle);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        snprintf(destination, destination_size, "not_available");
        return ESP_OK;
    }
    if (result != ESP_OK)
    {
        snprintf(destination, destination_size, "unavailable");
        return result;
    }
    destination[destination_size - 1U] = '\0';
    return ESP_OK;
}

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

esp_err_t ota_install_from_request(httpd_req_t *request)
{
    if (ota_update_in_progress)
    {
        return ota_send_error(request, "409 Conflict",
                              "{\"status\":\"busy\",\"message\":\"An OTA update is already in progress.\"}");
    }

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL)
    {
        ota_record_result("failed");
        return ota_send_error(request, "500 Internal Server Error",
                              "{\"status\":\"error\",\"message\":\"No inactive OTA partition is available.\"}");
    }

    if (request->content_len <= 0 || (size_t)request->content_len > update_partition->size)
    {
        ota_record_result("rejected");
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
        ota_record_result("failed");
        ESP_LOGE(TAG, "Unable to begin OTA update: %s", esp_err_to_name(result));
        return ota_send_error(request, "500 Internal Server Error",
                              "{\"status\":\"error\",\"message\":\"Unable to begin OTA update.\"}");
    }

    char receive_buffer[OTA_RECEIVE_BUFFER_SIZE];
    int remaining = request->content_len;
    unsigned int receive_timeout_retries = 0;
    while (remaining > 0)
    {
        const size_t receive_size = remaining < (int)sizeof(receive_buffer)
                                        ? (size_t)remaining
                                        : sizeof(receive_buffer);
        const int received = httpd_req_recv(request, receive_buffer, receive_size);
        if (received <= 0)
        {
            if (received == HTTPD_SOCK_ERR_TIMEOUT &&
                receive_timeout_retries < OTA_RECEIVE_TIMEOUT_RETRIES)
            {
                receive_timeout_retries++;
                ESP_LOGW(TAG, "OTA image receive timed out; retrying (%u/%u)",
                         receive_timeout_retries, OTA_RECEIVE_TIMEOUT_RETRIES);
                continue;
            }

            result = received == HTTPD_SOCK_ERR_TIMEOUT ? ESP_ERR_TIMEOUT : ESP_FAIL;
            ESP_LOGE(TAG, "OTA image receive failed: %s", esp_err_to_name(result));
            break;
        }

        receive_timeout_retries = 0;
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
        ota_record_result("failed");
        return ota_send_error(request, "422 Unprocessable Content",
                              "{\"status\":\"error\",\"message\":\"The uploaded file is not a valid ESP32-NUT firmware image.\"}");
    }

    ota_record_result("pending");
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
            ota_record_result("installed");
            ESP_LOGI(TAG, "New OTA image marked valid");
        }
        else
        {
            ota_record_result("failed");
            ESP_LOGE(TAG, "Unable to mark OTA image valid: %s", esp_err_to_name(result));
        }
    }
#endif
}
