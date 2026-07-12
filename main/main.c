#include <stdio.h>

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "esp32_nut_server";

void app_main(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;

    esp_chip_info(&chip_info);

    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to determine flash size");
        return;
    }

    ESP_LOGI(TAG, "ESP32 NUT Server starting");
    ESP_LOGI(TAG, "Chip cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "Chip revision: %d", chip_info.revision);
    ESP_LOGI(TAG, "Flash size: %lu MB", (unsigned long)(flash_size / (1024 * 1024)));
}
