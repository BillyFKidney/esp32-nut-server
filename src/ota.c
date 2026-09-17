/** @file ota.c @brief Validate and install ESP-IDF OTA application images. @see ota.h, esp_ota_ops.h, esp_app_desc.h, nvs.h */
#include "ota.h"

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bootloader_common.h"
#include "esp_err.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"
#include "psa/crypto.h"
#include "soc/soc.h"
#include "spi_flash_mmap.h"

#define TAG "nut-ota"
#define OTA_NVS_NAMESPACE "management"
#define OTA_LAST_RESULT_KEY "ota-result"
#define OTA_RECEIVE_BUFFER_SIZE 4096
#define OTA_RECEIVE_TIMEOUT_RETRIES 4
#define OTA_REBOOT_DELAY_MS 1000
#define OTA_REBOOT_TASK_STACK_SIZE 2048
#define OTA_REBOOT_TASK_PRIORITY 5
#define OTA_IMAGE_VERSION_BUFFER_SIZE (sizeof(((esp_app_desc_t *)0)->version) + 1U)
#define OTA_CHECK_RESPONSE_SIZE 192U
#define OTA_STAGE_IDENTIFIER_LENGTH 32U
#define OTA_STAGE_EXPIRY_US (10LL * 60LL * 1000LL * 1000LL)
#define OTA_IMAGE_MAX_SEGMENTS 16U
#define OTA_ROM_CHECKSUM_INITIAL 0xefU

#if CONFIG_SECURE_BOOT || CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#error "v2.9.2 PSRAM image validation must be extended before Secure Boot or anti-rollback is enabled"
#endif

static bool ota_update_in_progress;
static SemaphoreHandle_t ota_stage_lock;
static esp_timer_handle_t ota_stage_expiry_timer;

typedef struct
{
    uint8_t *image;
    size_t image_size;
    char identifier[OTA_STAGE_IDENTIFIER_LENGTH + 1U];
    char version[OTA_IMAGE_VERSION_BUFFER_SIZE];
    int64_t expires_at_us;
} OtaStagedImage;

static OtaStagedImage ota_staged_image;

typedef enum
{
    OTA_FAILURE_NONE,
    OTA_FAILURE_RECEIVE_TIMEOUT,
    OTA_FAILURE_RECEIVE,
    OTA_FAILURE_WRITE,
    OTA_FAILURE_VALIDATE,
    OTA_FAILURE_BOOT_SELECTION,
} OtaFailure;

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

static esp_err_t ota_send_failure(httpd_req_t *request, OtaFailure failure)
{
    switch (failure)
    {
    case OTA_FAILURE_RECEIVE_TIMEOUT:
        return ota_send_error(
            request, "408 Request Timeout",
            "{\"status\":\"error\",\"message\":\"Firmware upload timed out before completion. Disable automatic status refresh and try again.\"}");
    case OTA_FAILURE_RECEIVE:
        return ota_send_error(
            request, "400 Bad Request",
            "{\"status\":\"error\",\"message\":\"Firmware upload ended before the complete image was received.\"}");
    case OTA_FAILURE_WRITE:
        return ota_send_error(
            request, "500 Internal Server Error",
            "{\"status\":\"error\",\"message\":\"Unable to write the firmware upload to the inactive OTA partition.\"}");
    case OTA_FAILURE_VALIDATE:
        return ota_send_error(
            request, "422 Unprocessable Content",
            "{\"status\":\"error\",\"message\":\"The complete upload failed ESP32 application-image validation.\"}");
    case OTA_FAILURE_BOOT_SELECTION:
        return ota_send_error(
            request, "500 Internal Server Error",
            "{\"status\":\"error\",\"message\":\"Firmware was verified but could not be selected for the next boot.\"}");
    case OTA_FAILURE_NONE:
    default:
        return ota_send_error(request, "500 Internal Server Error",
                              "{\"status\":\"error\",\"message\":\"Firmware update failed.\"}");
    }
}

static void ota_copy_image_version(char *destination, size_t destination_size,
                                   const esp_app_desc_t *description)
{
    if (destination == NULL || destination_size == 0U)
    {
        return;
    }

    size_t destination_index = 0U;
    if (description != NULL)
    {
        for (size_t source_index = 0U;
             source_index < sizeof(description->version) &&
             description->version[source_index] != '\0' &&
             destination_index + 1U < destination_size;
             source_index++)
        {
            const unsigned char character =
                (unsigned char)description->version[source_index];
            destination[destination_index++] =
                character >= 0x20U && character <= 0x7eU && character != '"' &&
                        character != '\\'
                    ? (char)character
                    : '?';
        }
    }
    destination[destination_index] = '\0';

    if (destination_index == 0U)
    {
        snprintf(destination, destination_size, "unavailable");
    }
}

static bool ota_image_range_is_valid(size_t image_size, size_t offset, size_t length)
{
    return offset <= image_size && length <= image_size - offset;
}

static bool ota_image_segment_is_mapped(uint32_t load_address)
{
    return (load_address >= SOC_IROM_LOW && load_address < SOC_IROM_HIGH) ||
           (load_address >= SOC_DROM_LOW && load_address < SOC_DROM_HIGH);
}

static uint32_t ota_image_read_word(const uint8_t *source)
{
    uint32_t word;
    memcpy(&word, source, sizeof(word));
    return word;
}

static esp_err_t ota_validate_staged_image(const uint8_t *image, size_t image_size,
                                           char *version, size_t version_size)
{
    if (image == NULL || image_size < sizeof(esp_image_header_t) ||
        version == NULL || version_size == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_image_header_t header;
    memcpy(&header, image, sizeof(header));
    if (header.magic != ESP_IMAGE_HEADER_MAGIC ||
        header.segment_count == 0U || header.segment_count > OTA_IMAGE_MAX_SEGMENTS ||
        bootloader_common_check_chip_validity(&header, ESP_IMAGE_APPLICATION) != ESP_OK)
    {
        return ESP_ERR_IMAGE_INVALID;
    }

    psa_hash_operation_t hash = PSA_HASH_OPERATION_INIT;
    if (header.hash_appended != 0U &&
        psa_hash_setup(&hash, PSA_ALG_SHA_256) != PSA_SUCCESS)
    {
        return ESP_FAIL;
    }
    const bool calculate_hash = header.hash_appended != 0U;
    if (calculate_hash &&
        psa_hash_update(&hash, image, sizeof(header)) != PSA_SUCCESS)
    {
        psa_hash_abort(&hash);
        return ESP_FAIL;
    }

    size_t offset = sizeof(header);
    uint8_t checksum = OTA_ROM_CHECKSUM_INITIAL;
    for (uint8_t index = 0U; index < header.segment_count; index++)
    {
        esp_image_segment_header_t segment;
        if (!ota_image_range_is_valid(image_size, offset, sizeof(segment)))
        {
            psa_hash_abort(&hash);
            return ESP_ERR_IMAGE_INVALID;
        }
        memcpy(&segment, image + offset, sizeof(segment));
        if ((segment.data_len & 3U) != 0U || segment.data_len >= UINT32_C(0x1000000) ||
            !ota_image_range_is_valid(image_size, offset + sizeof(segment), segment.data_len) ||
            (ota_image_segment_is_mapped(segment.load_addr) &&
             ((offset + sizeof(segment)) % SPI_FLASH_MMU_PAGE_SIZE !=
              segment.load_addr % SPI_FLASH_MMU_PAGE_SIZE)))
        {
            psa_hash_abort(&hash);
            return ESP_ERR_IMAGE_INVALID;
        }
        if (calculate_hash &&
            psa_hash_update(&hash, image + offset, sizeof(segment)) != PSA_SUCCESS)
        {
            psa_hash_abort(&hash);
            return ESP_FAIL;
        }
        offset += sizeof(segment);

        if (index == 0U)
        {
            esp_app_desc_t description;
            if (segment.data_len < sizeof(description))
            {
                psa_hash_abort(&hash);
                return ESP_ERR_IMAGE_INVALID;
            }
            memcpy(&description, image + offset, sizeof(description));
            if (description.magic_word != ESP_APP_DESC_MAGIC_WORD ||
                bootloader_common_check_efuse_blk_validity(
                    description.min_efuse_blk_rev_full,
                    description.max_efuse_blk_rev_full) != ESP_OK)
            {
                psa_hash_abort(&hash);
                return ESP_ERR_IMAGE_INVALID;
            }
            ota_copy_image_version(version, version_size, &description);
        }

        for (size_t word_offset = 0U; word_offset < segment.data_len;
             word_offset += sizeof(uint32_t))
        {
            const uint32_t word = ota_image_read_word(image + offset + word_offset);
            checksum ^= (uint8_t)word ^ (uint8_t)(word >> 8U) ^
                        (uint8_t)(word >> 16U) ^ (uint8_t)(word >> 24U);
        }
        if (calculate_hash &&
            psa_hash_update(&hash, image + offset, segment.data_len) != PSA_SUCCESS)
        {
            psa_hash_abort(&hash);
            return ESP_FAIL;
        }
        offset += segment.data_len;
    }

    const size_t checksum_length = ((offset + 1U + 15U) & ~((size_t)15U)) - offset;
    if (!ota_image_range_is_valid(image_size, offset, checksum_length) ||
        image[offset + checksum_length - 1U] != checksum)
    {
        psa_hash_abort(&hash);
        return ESP_ERR_IMAGE_INVALID;
    }
    if (calculate_hash &&
        psa_hash_update(&hash, image + offset, checksum_length) != PSA_SUCCESS)
    {
        psa_hash_abort(&hash);
        return ESP_FAIL;
    }
    offset += checksum_length;

    if (calculate_hash)
    {
        uint8_t digest[ESP_IMAGE_HASH_LEN];
        size_t digest_length = 0U;
        const psa_status_t finish_result =
            psa_hash_finish(&hash, digest, sizeof(digest), &digest_length);
        if (finish_result != PSA_SUCCESS || digest_length != sizeof(digest) ||
            !ota_image_range_is_valid(image_size, offset, sizeof(digest)) ||
            memcmp(digest, image + offset, sizeof(digest)) != 0)
        {
            psa_hash_abort(&hash);
            return ESP_ERR_IMAGE_INVALID;
        }
        offset += sizeof(digest);
    }
    else
    {
        psa_hash_abort(&hash);
    }

    return offset == image_size ? ESP_OK : ESP_ERR_IMAGE_INVALID;
}

static void ota_clear_staged_image(void)
{
    if (ota_staged_image.image != NULL)
    {
        mbedtls_platform_zeroize(ota_staged_image.image, ota_staged_image.image_size);
        free(ota_staged_image.image);
    }
    memset(&ota_staged_image, 0, sizeof(ota_staged_image));
}

static bool ota_staged_image_is_expired(void)
{
    return ota_staged_image.image != NULL &&
           esp_timer_get_time() >= ota_staged_image.expires_at_us;
}

static void ota_stage_expiry_callback(void *argument)
{
    (void)argument;
    if (ota_stage_lock != NULL && xSemaphoreTake(ota_stage_lock, 0) == pdTRUE)
    {
        if (ota_staged_image_is_expired())
        {
            ESP_LOGI(TAG, "Discarding expired PSRAM firmware stage");
            ota_clear_staged_image();
        }
        xSemaphoreGive(ota_stage_lock);
    }
    else if (ota_stage_expiry_timer != NULL)
    {
        /* A request may briefly hold the lock when expiry fires. Retry so the
         * checked image is still discarded even if that request stalls. */
        esp_timer_start_once(ota_stage_expiry_timer, 1000LL * 1000LL);
    }
}

void ota_init(void)
{
    ota_stage_lock = xSemaphoreCreateMutex();
    if (ota_stage_lock == NULL)
    {
        ESP_LOGE(TAG, "Unable to create OTA stage lock");
        return;
    }
    const esp_timer_create_args_t expiry_timer = {
        .callback = ota_stage_expiry_callback,
        .name = "ota-stage-expiry",
    };
    const esp_err_t result = esp_timer_create(&expiry_timer, &ota_stage_expiry_timer);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to create OTA stage expiry timer: %s", esp_err_to_name(result));
    }
}

static esp_err_t ota_take_stage_lock(httpd_req_t *request)
{
    if (ota_stage_lock == NULL || xSemaphoreTake(ota_stage_lock, 0) != pdTRUE)
    {
        return ota_send_error(request, "409 Conflict",
                              "{\"status\":\"busy\",\"message\":\"An OTA operation is already in progress.\"}");
    }
    if (ota_staged_image_is_expired())
    {
        ota_clear_staged_image();
    }
    return ESP_OK;
}

static void ota_give_stage_lock(void)
{
    xSemaphoreGive(ota_stage_lock);
}

static esp_err_t ota_receive_staged_image(httpd_req_t *request, uint8_t *image,
                                           size_t image_size, OtaFailure *failure)
{
    size_t received_total = 0U;
    unsigned int timeout_retries = 0U;
    while (received_total < image_size)
    {
        const int received = httpd_req_recv(request, (char *)image + received_total,
                                            image_size - received_total);
        if (received <= 0)
        {
            if (received == HTTPD_SOCK_ERR_TIMEOUT &&
                timeout_retries < OTA_RECEIVE_TIMEOUT_RETRIES)
            {
                timeout_retries++;
                continue;
            }
            *failure = received == HTTPD_SOCK_ERR_TIMEOUT ? OTA_FAILURE_RECEIVE_TIMEOUT
                                                           : OTA_FAILURE_RECEIVE;
            return received == HTTPD_SOCK_ERR_TIMEOUT ? ESP_ERR_TIMEOUT : ESP_FAIL;
        }
        received_total += (size_t)received;
        timeout_retries = 0U;
    }
    return ESP_OK;
}

static void ota_generate_stage_identifier(char *identifier, size_t identifier_size)
{
    snprintf(identifier, identifier_size, "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32,
             esp_random(), esp_random(), esp_random(), esp_random());
}

static esp_err_t ota_send_staged_response(httpd_req_t *request, const OtaStagedImage *stage)
{
    char response[OTA_CHECK_RESPONSE_SIZE];
    const int length = snprintf(
        response, sizeof(response),
        "{\"status\":\"checked\",\"firmware_version\":\"%s\",\"stage\":\"%s\","
        "\"message\":\"Firmware image verified in PSRAM. It is ready to install.\"}",
        stage->version, stage->identifier);
    if (length < 0 || (size_t)length >= sizeof(response))
    {
        return ota_send_error(request, "500 Internal Server Error",
                              "{\"status\":\"error\",\"message\":\"Unable to report the checked firmware image.\"}");
    }
    ota_set_response_headers(request);
    return httpd_resp_sendstr(request, response);
}

static void ota_reboot_task(void *parameter)
{
    (void)parameter;
    vTaskDelay(pdMS_TO_TICKS(OTA_REBOOT_DELAY_MS));
    ESP_LOGI(TAG, "Restarting into the OTA image");
    esp_restart();
}

static esp_err_t ota_receive_image(httpd_req_t *request,
                                   esp_ota_handle_t update_handle,
                                   OtaFailure *failure)
{
    char *receive_buffer = heap_caps_malloc(OTA_RECEIVE_BUFFER_SIZE,
                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (receive_buffer == NULL)
    {
        receive_buffer = malloc(OTA_RECEIVE_BUFFER_SIZE);
    }
    if (receive_buffer == NULL)
    {
        *failure = OTA_FAILURE_WRITE;
        return ESP_ERR_NO_MEM;
    }
    int remaining = request->content_len;
    unsigned int receive_timeout_retries = 0;
    esp_err_t result = ESP_OK;

    while (remaining > 0)
    {
        const size_t receive_size = remaining < (int)OTA_RECEIVE_BUFFER_SIZE
                                        ? (size_t)remaining
                                        : OTA_RECEIVE_BUFFER_SIZE;
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
            *failure = received == HTTPD_SOCK_ERR_TIMEOUT
                           ? OTA_FAILURE_RECEIVE_TIMEOUT
                           : OTA_FAILURE_RECEIVE;
            ESP_LOGE(TAG, "OTA image receive failed: %s", esp_err_to_name(result));
            break;
        }

        receive_timeout_retries = 0;
        result = esp_ota_write(update_handle, receive_buffer, received);
        if (result != ESP_OK)
        {
            *failure = result == ESP_ERR_OTA_VALIDATE_FAILED
                           ? OTA_FAILURE_VALIDATE
                           : OTA_FAILURE_WRITE;
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
            *failure = result == ESP_ERR_OTA_VALIDATE_FAILED
                           ? OTA_FAILURE_VALIDATE
                           : OTA_FAILURE_WRITE;
            ESP_LOGE(TAG, "OTA image verification failed: %s", esp_err_to_name(result));
        }
    }
    else
    {
        esp_ota_abort(update_handle);
    }

    mbedtls_platform_zeroize(receive_buffer, OTA_RECEIVE_BUFFER_SIZE);
    free(receive_buffer);
    return result;
}

esp_err_t ota_stage_from_request(httpd_req_t *request)
{
    esp_err_t result = ota_take_stage_lock(request);
    if (result != ESP_OK)
    {
        return result;
    }
    if (ota_update_in_progress)
    {
        ota_give_stage_lock();
        return ota_send_error(request, "409 Conflict",
                              "{\"status\":\"busy\",\"message\":\"An OTA update is already in progress.\"}");
    }
    ota_update_in_progress = true;

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL || request->content_len <= 0 ||
        (size_t)request->content_len > update_partition->size)
    {
        ota_update_in_progress = false;
        ota_give_stage_lock();
        return ota_send_error(request, "413 Payload Too Large",
                              "{\"status\":\"error\",\"message\":\"The uploaded image does not fit in the inactive OTA partition.\"}");
    }

    const size_t image_size = (size_t)request->content_len;
    uint8_t *image = heap_caps_malloc(image_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (image == NULL)
    {
        ota_update_in_progress = false;
        ota_give_stage_lock();
        return ota_send_error(request, "503 Service Unavailable",
                              "{\"status\":\"error\",\"message\":\"PSRAM is unavailable for the firmware upload.\"}");
    }

    OtaFailure failure = OTA_FAILURE_NONE;
    result = ota_receive_staged_image(request, image, image_size, &failure);
    char version[OTA_IMAGE_VERSION_BUFFER_SIZE] = {0};
    if (result == ESP_OK)
    {
        result = ota_validate_staged_image(image, image_size, version, sizeof(version));
        if (result != ESP_OK)
        {
            failure = OTA_FAILURE_VALIDATE;
        }
    }
    if (result != ESP_OK)
    {
        mbedtls_platform_zeroize(image, image_size);
        free(image);
        ota_update_in_progress = false;
        ota_give_stage_lock();
        return ota_send_failure(request, failure);
    }

    if (ota_stage_expiry_timer != NULL)
    {
        esp_timer_stop(ota_stage_expiry_timer);
    }
    ota_clear_staged_image();
    ota_staged_image.image = image;
    ota_staged_image.image_size = image_size;
    snprintf(ota_staged_image.version, sizeof(ota_staged_image.version), "%s", version);
    ota_generate_stage_identifier(ota_staged_image.identifier,
                                  sizeof(ota_staged_image.identifier));
    ota_staged_image.expires_at_us = esp_timer_get_time() + OTA_STAGE_EXPIRY_US;
    if (ota_stage_expiry_timer == NULL ||
        esp_timer_start_once(ota_stage_expiry_timer, OTA_STAGE_EXPIRY_US) != ESP_OK)
    {
        ota_clear_staged_image();
        ota_update_in_progress = false;
        ota_give_stage_lock();
        return ota_send_error(request, "500 Internal Server Error",
                              "{\"status\":\"error\",\"message\":\"Unable to retain the checked firmware image.\"}");
    }

    ota_update_in_progress = false;
    const esp_err_t response_result = ota_send_staged_response(request, &ota_staged_image);
    ota_give_stage_lock();
    return response_result;
}

esp_err_t ota_install_staged_from_request(httpd_req_t *request,
                                          const char *stage_identifier)
{
    esp_err_t result = ota_take_stage_lock(request);
    if (result != ESP_OK)
    {
        return result;
    }
    if (ota_update_in_progress || ota_staged_image.image == NULL ||
        strcmp(ota_staged_image.identifier, stage_identifier) != 0)
    {
        ota_give_stage_lock();
        return ota_send_error(request, "409 Conflict",
                              "{\"status\":\"error\",\"message\":\"No matching checked firmware image is available.\"}");
    }
    ota_update_in_progress = true;
    if (ota_stage_expiry_timer != NULL)
    {
        esp_timer_stop(ota_stage_expiry_timer);
    }

    char version[OTA_IMAGE_VERSION_BUFFER_SIZE] = {0};
    result = ota_validate_staged_image(ota_staged_image.image, ota_staged_image.image_size,
                                       version, sizeof(version));
    if (result != ESP_OK)
    {
        ota_clear_staged_image();
        ota_update_in_progress = false;
        ota_give_stage_lock();
        ota_record_result("rejected");
        return ota_send_failure(request, OTA_FAILURE_VALIDATE);
    }

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t update_handle = 0;
    if (update_partition == NULL ||
        esp_ota_begin(update_partition, ota_staged_image.image_size, &update_handle) != ESP_OK)
    {
        ota_clear_staged_image();
        ota_update_in_progress = false;
        ota_give_stage_lock();
        ota_record_result("failed");
        return ota_send_error(request, "500 Internal Server Error",
                              "{\"status\":\"error\",\"message\":\"Unable to begin OTA update.\"}");
    }

    OtaFailure failure = OTA_FAILURE_NONE;
    for (size_t offset = 0U; offset < ota_staged_image.image_size;)
    {
        const size_t chunk_size = ota_staged_image.image_size - offset > OTA_RECEIVE_BUFFER_SIZE
                                      ? OTA_RECEIVE_BUFFER_SIZE
                                      : ota_staged_image.image_size - offset;
        result = esp_ota_write(update_handle, ota_staged_image.image + offset, chunk_size);
        if (result != ESP_OK)
        {
            failure = OTA_FAILURE_WRITE;
            esp_ota_abort(update_handle);
            break;
        }
        offset += chunk_size;
    }
    if (result == ESP_OK)
    {
        result = esp_ota_end(update_handle);
        if (result != ESP_OK)
        {
            failure = result == ESP_ERR_OTA_VALIDATE_FAILED ? OTA_FAILURE_VALIDATE
                                                             : OTA_FAILURE_WRITE;
        }
    }
    if (result == ESP_OK)
    {
        result = esp_ota_set_boot_partition(update_partition);
        if (result != ESP_OK)
        {
            failure = OTA_FAILURE_BOOT_SELECTION;
        }
    }

    ota_clear_staged_image();
    ota_update_in_progress = false;
    ota_give_stage_lock();
    if (result != ESP_OK)
    {
        ota_record_result("failed");
        return ota_send_failure(request, failure);
    }

    ota_record_result("pending");
    ota_set_response_headers(request);
    const esp_err_t response_result = httpd_resp_sendstr(
        request, "{\"status\":\"installed\",\"message\":\"Firmware verified. Restarting now.\"}");
    if (response_result == ESP_OK &&
        xTaskCreate(ota_reboot_task, "ota-reboot", OTA_REBOOT_TASK_STACK_SIZE, NULL,
                    OTA_REBOOT_TASK_PRIORITY, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Unable to schedule OTA reboot");
        esp_restart();
    }
    return response_result;
}

static esp_err_t ota_process_from_request(httpd_req_t *request)
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

    OtaFailure failure = OTA_FAILURE_NONE;
    result = ota_receive_image(request, update_handle, &failure);

    if (result == ESP_OK)
    {
        result = esp_ota_set_boot_partition(update_partition);
        if (result != ESP_OK)
        {
            failure = OTA_FAILURE_BOOT_SELECTION;
            ESP_LOGE(TAG, "Unable to select OTA partition for boot: %s", esp_err_to_name(result));
        }
    }

    ota_update_in_progress = false;
    if (result != ESP_OK)
    {
        ota_record_result("failed");
        return ota_send_failure(request, failure);
    }

    ota_record_result("pending");
    ESP_LOGI(TAG, "OTA image verified and selected for the next boot");
    ota_set_response_headers(request);
    const esp_err_t response_result = httpd_resp_sendstr(
        request, "{\"status\":\"installed\",\"message\":\"Firmware verified. Restarting now.\"}");
    if (response_result == ESP_OK &&
        xTaskCreate(ota_reboot_task, "ota-reboot", OTA_REBOOT_TASK_STACK_SIZE, NULL,
                    OTA_REBOOT_TASK_PRIORITY, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Unable to schedule OTA reboot");
        esp_restart();
    }
    return response_result;
}

esp_err_t ota_install_from_request(httpd_req_t *request)
{
    const esp_err_t lock_result = ota_take_stage_lock(request);
    if (lock_result != ESP_OK)
    {
        return lock_result;
    }

    const esp_err_t result = ota_process_from_request(request);
    ota_give_stage_lock();
    return result;
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
