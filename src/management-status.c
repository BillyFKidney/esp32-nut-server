/** @file management-status.c @brief Collect NUT and hardware diagnostics for management status. @see management-status.h, drivers/dstate.h, esp_heap_caps.h, driver/temperature_sensor.h */
#include "management-status.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "driver/temperature_sensor.h"
#include "drivers/dstate.h"
#include "drivers/espusb.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "nut-diagnostics.h"

#define TAG "nut-management"

#define MANAGEMENT_STATUS_BOARD_PROFILE "YD-ESP32-23"
#define MANAGEMENT_STATUS_MODULE_PROFILE "ESP32-S3-WROOM-1-N16R8"

#if CONFIG_ESPTOOLPY_FLASHSIZE_1MB
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES (1U * 1024U * 1024U)
#elif CONFIG_ESPTOOLPY_FLASHSIZE_2MB
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES (2U * 1024U * 1024U)
#elif CONFIG_ESPTOOLPY_FLASHSIZE_4MB
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES (4U * 1024U * 1024U)
#elif CONFIG_ESPTOOLPY_FLASHSIZE_8MB
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES (8U * 1024U * 1024U)
#elif CONFIG_ESPTOOLPY_FLASHSIZE_16MB
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES (16U * 1024U * 1024U)
#elif CONFIG_ESPTOOLPY_FLASHSIZE_32MB
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES (32U * 1024U * 1024U)
#elif CONFIG_ESPTOOLPY_FLASHSIZE_64MB
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES (64U * 1024U * 1024U)
#elif CONFIG_ESPTOOLPY_FLASHSIZE_128MB
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES (128U * 1024U * 1024U)
#else
#define MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES 0U
#endif

#if CONFIG_SPIRAM_MODE_OCT
#define MANAGEMENT_STATUS_PSRAM_MODE "octal"
#elif CONFIG_SPIRAM_MODE_QUAD
#define MANAGEMENT_STATUS_PSRAM_MODE "quad"
#else
#define MANAGEMENT_STATUS_PSRAM_MODE "unavailable"
#endif

#if CONFIG_SPIRAM
#define MANAGEMENT_STATUS_PSRAM_SPEED_MHZ CONFIG_SPIRAM_SPEED
#else
#define MANAGEMENT_STATUS_PSRAM_SPEED_MHZ 0
#endif

static bool management_status_hardware_initialized;
static uint32_t management_status_flash_size_bytes;
static temperature_sensor_handle_t management_status_temperature_sensor;

extern const char *upsname;

static void management_status_copy_nut_value(const char *name, char *destination,
                                             size_t destination_size)
{
    const char *value = dstate_getinfo(name);
    if (value == NULL || *value == '\0')
    {
        snprintf(destination, destination_size, "unavailable");
        return;
    }
    snprintf(destination, destination_size, "%s", value);
}

void management_status_collect_nut_snapshot(ManagementStatusNutSnapshot *snapshot)
{
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->disconnect_simulated =
        nut_diagnostics_disconnect_simulation_active();
    snapshot->stale = dstate_is_stale() != 0 || !usb_hid_device_ready() ||
                      snapshot->disconnect_simulated;
    const char *status = dstate_getinfo("ups.status");
    snapshot->available = status != NULL && !snapshot->stale;
    snprintf(snapshot->ups, sizeof(snapshot->ups), "%s",
             upsname != NULL && *upsname != '\0' ? upsname : "cyberpower");
    if (snapshot->stale)
    {
        snprintf(snapshot->manufacturer, sizeof(snapshot->manufacturer), "unavailable");
        snprintf(snapshot->model, sizeof(snapshot->model), "unavailable");
        snprintf(snapshot->serial, sizeof(snapshot->serial), "unavailable");
        snprintf(snapshot->status, sizeof(snapshot->status), "unavailable");
        snprintf(snapshot->battery_type, sizeof(snapshot->battery_type), "unavailable");
        snprintf(snapshot->battery_mfr_date, sizeof(snapshot->battery_mfr_date), "unavailable");
        snprintf(snapshot->ups_temperature, sizeof(snapshot->ups_temperature), "unavailable");
        snprintf(snapshot->battery_charge, sizeof(snapshot->battery_charge), "unavailable");
        snprintf(snapshot->battery_runtime, sizeof(snapshot->battery_runtime), "unavailable");
        snprintf(snapshot->battery_voltage, sizeof(snapshot->battery_voltage), "unavailable");
        snprintf(snapshot->load, sizeof(snapshot->load), "unavailable");
        snprintf(snapshot->input_voltage, sizeof(snapshot->input_voltage), "unavailable");
        snprintf(snapshot->output_voltage, sizeof(snapshot->output_voltage), "unavailable");
        snprintf(snapshot->ups_power, sizeof(snapshot->ups_power), "unavailable");
        snprintf(snapshot->ups_realpower, sizeof(snapshot->ups_realpower), "unavailable");
        snprintf(snapshot->ups_firmware, sizeof(snapshot->ups_firmware), "unavailable");
        return;
    }
    management_status_copy_nut_value("device.mfr", snapshot->manufacturer,
                                     sizeof(snapshot->manufacturer));
    if (strcmp(snapshot->manufacturer, "unavailable") == 0)
    {
        management_status_copy_nut_value("ups.mfr", snapshot->manufacturer,
                                         sizeof(snapshot->manufacturer));
    }
    management_status_copy_nut_value("device.model", snapshot->model,
                                     sizeof(snapshot->model));
    if (strcmp(snapshot->model, "unavailable") == 0)
    {
        management_status_copy_nut_value("ups.model", snapshot->model,
                                         sizeof(snapshot->model));
    }
    management_status_copy_nut_value("device.serial", snapshot->serial,
                                     sizeof(snapshot->serial));
    if (strcmp(snapshot->serial, "unavailable") == 0)
    {
        management_status_copy_nut_value("ups.serial", snapshot->serial,
                                         sizeof(snapshot->serial));
    }
    management_status_copy_nut_value("ups.status", snapshot->status,
                                     sizeof(snapshot->status));
    management_status_copy_nut_value("battery.type", snapshot->battery_type,
                                     sizeof(snapshot->battery_type));
    management_status_copy_nut_value("battery.mfr.date", snapshot->battery_mfr_date,
                                     sizeof(snapshot->battery_mfr_date));
    management_status_copy_nut_value("ups.temperature", snapshot->ups_temperature,
                                     sizeof(snapshot->ups_temperature));
    management_status_copy_nut_value("battery.charge", snapshot->battery_charge,
                                     sizeof(snapshot->battery_charge));
    management_status_copy_nut_value("battery.runtime", snapshot->battery_runtime,
                                     sizeof(snapshot->battery_runtime));
    management_status_copy_nut_value("battery.voltage", snapshot->battery_voltage,
                                     sizeof(snapshot->battery_voltage));
    management_status_copy_nut_value("ups.load", snapshot->load,
                                     sizeof(snapshot->load));
    management_status_copy_nut_value("input.voltage", snapshot->input_voltage,
                                     sizeof(snapshot->input_voltage));
    management_status_copy_nut_value("output.voltage", snapshot->output_voltage,
                                     sizeof(snapshot->output_voltage));
    management_status_copy_nut_value("ups.power", snapshot->ups_power,
                                     sizeof(snapshot->ups_power));
    management_status_copy_nut_value("ups.realpower", snapshot->ups_realpower,
                                     sizeof(snapshot->ups_realpower));
    management_status_copy_nut_value("ups.firmware", snapshot->ups_firmware,
                                     sizeof(snapshot->ups_firmware));
}

const char *management_status_chip_model_name(esp_chip_model_t model)
{
    switch (model)
    {
    case CHIP_ESP32:
        return "ESP32";
    case CHIP_ESP32S2:
        return "ESP32-S2";
    case CHIP_ESP32S3:
        return "ESP32-S3";
    case CHIP_ESP32C2:
        return "ESP32-C2";
    case CHIP_ESP32C3:
        return "ESP32-C3";
    case CHIP_ESP32C5:
        return "ESP32-C5";
    case CHIP_ESP32C6:
        return "ESP32-C6";
    case CHIP_ESP32H2:
        return "ESP32-H2";
    case CHIP_ESP32P4:
        return "ESP32-P4";
    case CHIP_ESP32C61:
        return "ESP32-C61";
    case CHIP_ESP32H21:
        return "ESP32-H21";
    case CHIP_ESP32H4:
        return "ESP32-H4";
    default:
        return "unknown";
    }
}

void management_status_initialize_hardware_diagnostics(void)
{
    if (management_status_hardware_initialized)
    {
        return;
    }
    management_status_hardware_initialized = true;

    uint32_t detected_flash_size = 0;
    if (esp_flash_get_physical_size(NULL, &detected_flash_size) == ESP_OK)
    {
        management_status_flash_size_bytes = detected_flash_size;
    }
    else
    {
        management_status_flash_size_bytes = MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES;
        ESP_LOGW(TAG, "Unable to detect physical flash size; using configured size");
    }

    const temperature_sensor_config_t temperature_configuration =
        TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    esp_err_t result = temperature_sensor_install(&temperature_configuration,
                                                  &management_status_temperature_sensor);
    if (result != ESP_OK)
    {
        management_status_temperature_sensor = NULL;
        ESP_LOGW(TAG, "Internal chip temperature is unavailable: %s",
                 esp_err_to_name(result));
        return;
    }

    result = temperature_sensor_enable(management_status_temperature_sensor);
    if (result != ESP_OK)
    {
        ESP_LOGW(TAG, "Unable to enable internal chip temperature: %s",
                 esp_err_to_name(result));
        temperature_sensor_uninstall(management_status_temperature_sensor);
        management_status_temperature_sensor = NULL;
    }
}

void management_status_collect_hardware_snapshot(ManagementStatusHardwareSnapshot *snapshot)
{
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->board_profile = MANAGEMENT_STATUS_BOARD_PROFILE;
    snapshot->module_profile = MANAGEMENT_STATUS_MODULE_PROFILE;
    snapshot->flash_mode = CONFIG_ESPTOOLPY_FLASHMODE;
    snapshot->flash_frequency = CONFIG_ESPTOOLPY_FLASHFREQ;
    snapshot->psram_mode = MANAGEMENT_STATUS_PSRAM_MODE;
    snapshot->psram_frequency_mhz = MANAGEMENT_STATUS_PSRAM_SPEED_MHZ;
    esp_chip_info(&snapshot->chip);
    snapshot->flash_size_bytes = management_status_flash_size_bytes != 0
                                     ? management_status_flash_size_bytes
                                     : MANAGEMENT_STATUS_COMPILED_FLASH_SIZE_BYTES;
    snapshot->psram_size_bytes = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    snapshot->free_internal_heap_bytes =
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    snapshot->free_psram_bytes =
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    snapshot->minimum_free_heap_bytes = esp_get_minimum_free_heap_size();

    if (management_status_temperature_sensor != NULL)
    {
        float temperature_celsius = 0.0f;
        if (temperature_sensor_get_celsius(management_status_temperature_sensor,
                                            &temperature_celsius) == ESP_OK &&
            isfinite(temperature_celsius))
        {
            snapshot->chip_temperature_available = true;
            snapshot->chip_temperature_celsius = temperature_celsius;
        }
    }
}
