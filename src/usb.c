#include "usb/usb_host.h"
#include "usb/usb_helpers.h"
#include "drivers/espusb.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#define CLIENT_NUM_EVENT_MSG 5

#define DEV_MAX_COUNT 128

#define USB_DISCOVERY_CLIENT_EVENT_MSGS 5
#define USB_DISCOVERY_TASK_STACK_SIZE 4096
#define USB_DISCOVERY_TASK_PRIORITY 3

static const char *TAG = "CLASS";

typedef enum
{
    USB_DISCOVERY_ACTION_NONE = 0,
    USB_DISCOVERY_ACTION_OPEN_DEVICE = BIT0,
    USB_DISCOVERY_ACTION_CLOSE_DEVICE = BIT1,
} UsbDiscoveryAction;

typedef struct
{
    usb_host_client_handle_t client_handle;
    usb_device_handle_t device_handle;
    uint8_t device_address;
    uint32_t actions;
} UsbDiscoveryState;

static const char *usb_speed_name(usb_speed_t speed)
{
    switch (speed)
    {
    case USB_SPEED_LOW:
        return "low";
    case USB_SPEED_FULL:
        return "full";
    case USB_SPEED_HIGH:
        return "high";
    default:
        return "unknown";
    }
}

static void usb_discovery_client_event_callback(
    const usb_host_client_event_msg_t *event_msg,
    void *arg)
{
    UsbDiscoveryState *state = (UsbDiscoveryState *)arg;

    switch (event_msg->event)
    {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
        state->device_address = event_msg->new_dev.address;
        state->actions |= USB_DISCOVERY_ACTION_OPEN_DEVICE;
        break;
    case USB_HOST_CLIENT_EVENT_DEV_GONE:
        if (state->device_handle == event_msg->dev_gone.dev_hdl)
        {
            state->actions = USB_DISCOVERY_ACTION_CLOSE_DEVICE;
        }
        break;
    case USB_HOST_CLIENT_EVENT_DEV_SUSPENDED:
        ESP_LOGI(TAG, "USB device suspended");
        break;
    case USB_HOST_CLIENT_EVENT_DEV_RESUMED:
        ESP_LOGI(TAG, "USB device resumed");
        break;
    default:
        ESP_LOGW(TAG, "Unhandled USB client event: %d", event_msg->event);
        break;
    }
}

static void usb_discovery_print_device(UsbDiscoveryState *state)
{
    usb_device_info_t device_info;
    const usb_device_desc_t *device_descriptor = NULL;
    const usb_config_desc_t *config_descriptor = NULL;
    esp_err_t err;

    err = usb_host_device_open(
        state->client_handle,
        state->device_address,
        &state->device_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to open USB device %u: %s",
                 state->device_address, esp_err_to_name(err));
        state->device_address = 0;
        return;
    }

    ESP_LOGI(TAG, "USB device connected at address %u", state->device_address);

    err = usb_host_device_info(state->device_handle, &device_info);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Speed: %s", usb_speed_name(device_info.speed));
        ESP_LOGI(TAG, "Active configuration: %u", device_info.bConfigurationValue);
        ESP_LOGI(TAG, "Endpoint zero maximum packet size: %u", device_info.bMaxPacketSize0);
    }
    else
    {
        ESP_LOGE(TAG, "Unable to read USB device information: %s", esp_err_to_name(err));
    }

    err = usb_host_get_device_descriptor(state->device_handle, &device_descriptor);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "VID:PID %04x:%04x",
                 device_descriptor->idVendor, device_descriptor->idProduct);
        ESP_LOGI(TAG, "Device release: %x.%02x",
                 device_descriptor->bcdDevice >> 8,
                 device_descriptor->bcdDevice & 0xff);
        usb_print_device_descriptor(device_descriptor);
    }
    else
    {
        ESP_LOGE(TAG, "Unable to read USB device descriptor: %s", esp_err_to_name(err));
    }

    err = usb_host_get_active_config_descriptor(state->device_handle, &config_descriptor);
    if (err == ESP_OK)
    {
        usb_print_config_descriptor(config_descriptor, NULL);
    }
    else
    {
        ESP_LOGE(TAG, "Unable to read USB configuration descriptor: %s", esp_err_to_name(err));
    }

    err = usb_host_device_info(state->device_handle, &device_info);
    if (err == ESP_OK)
    {
        if (device_info.str_desc_manufacturer != NULL)
        {
            ESP_LOGI(TAG, "Manufacturer:");
            usb_print_string_descriptor(device_info.str_desc_manufacturer);
        }
        if (device_info.str_desc_product != NULL)
        {
            ESP_LOGI(TAG, "Product:");
            usb_print_string_descriptor(device_info.str_desc_product);
        }
        if (device_info.str_desc_serial_num != NULL)
        {
            ESP_LOGI(TAG, "Serial number:");
            usb_print_string_descriptor(device_info.str_desc_serial_num);
        }
    }
}

static void usb_discovery_close_device(UsbDiscoveryState *state)
{
    esp_err_t err;

    if (state->device_handle == NULL)
    {
        return;
    }

    err = usb_host_device_close(state->client_handle, state->device_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to close disconnected USB device: %s", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "USB device disconnected cleanly");
    }

    state->device_handle = NULL;
    state->device_address = 0;
}

static void usb_discovery_task(void *arg)
{
    UsbDiscoveryState state = {0};
    const usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = USB_DISCOVERY_CLIENT_EVENT_MSGS,
        .async = {
            .client_event_callback = usb_discovery_client_event_callback,
            .callback_arg = &state,
        },
    };

    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &state.client_handle));
    ESP_LOGI(TAG, "Read-only USB discovery client ready; waiting for a device");

    while (true)
    {
        esp_err_t err = usb_host_client_handle_events(state.client_handle, portMAX_DELAY);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "USB discovery event handling failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        uint32_t actions = state.actions;
        state.actions = USB_DISCOVERY_ACTION_NONE;

        if (actions & USB_DISCOVERY_ACTION_CLOSE_DEVICE)
        {
            usb_discovery_close_device(&state);
        }
        if (actions & USB_DISCOVERY_ACTION_OPEN_DEVICE)
        {
            usb_discovery_print_device(&state);
        }
    }
}


// HID

typedef struct
{
    /* HID Host - Device related info */
    struct
    {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

QueueHandle_t espusb_event_queue = NULL;

hid_host_device_handle_t espusb_hid_device_handle = NULL;

/**
 * @brief Start USB Host install and handle common USB host library events while app pin not low
 *
 * @param[in] arg  Not used
 */
static void usb_lib_task(void *arg)
{
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskNotifyGive(arg);

    while (true)
    {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // In this example, there is only one client registered
        // So, once we deregister the client, this call must succeed with ESP_OK
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
        {
            ESP_ERROR_CHECK(usb_host_device_free_all());
            break;
        }
    }

    ESP_LOGI(TAG, "USB shutdown");
    // Clean up USB Host
    vTaskDelay(10); // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

/**
 * @brief USB HID Host interface callback
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host interface event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg)
{
    uint8_t data[64] = {0};
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event)
    {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                                  data,
                                                                  64,
                                                                  &data_length));

        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class)
        {
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto)
            {
                //           hid_host_keyboard_report_callback(data, data_length);
            }
            else if (HID_PROTOCOL_MOUSE == dev_params.proto)
            {
                //           hid_host_mouse_report_callback(data, data_length);
            }
        }
        else
        {
            //       hid_host_generic_report_callback(data, data_length);
        }

        break;
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%d' DISCONNECTED",
                 dev_params.proto);
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        if (espusb_hid_device_handle == hid_device_handle)
        {
            espusb_hid_device_handle = NULL;
        }
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGI(TAG, "HID Device, protocol '%d' TRANSFER_ERROR",
                 dev_params.proto);
        break;
    default:
        ESP_LOGE(TAG, "HID Device, protocol '%d' Unhandled event",
                 dev_params.proto);
        break;
    }
}

/**
 * @brief HID Host Device callback
 *
 * Puts new HID Device event to the queue
 *
 * @param[in] hid_device_handle HID Device handle
 * @param[in] event             HID Device event
 * @param[in] arg               Not used
 */
void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event,
                              void *arg)
{
    if (event == HID_HOST_DRIVER_EVENT_CONNECTED)
    {
        hid_host_dev_params_t dev_params;
        ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

        ESP_LOGI(TAG, "HID Device, protocol '%d' CONNECTED", dev_params.proto);

        const hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL};

        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));

        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));

        espusb_hid_device_handle = hid_device_handle;

        ESP_LOGI(TAG, "HID Device, protocol '%d' STARTED", dev_params.proto);
        return;
    }

    const app_event_queue_t evt_queue = {
        // HID Host Device related info
        .hid_host_device.handle = hid_device_handle,
        .hid_host_device.event = event,
        .hid_host_device.arg = arg};

    if (espusb_event_queue)
    {
        xQueueSend(espusb_event_queue, &evt_queue, 0);
    }
}

void hidHostInstall()
{
    BaseType_t task_created;

    /*
     * Create usb_lib_task to:
     * - initialize USB Host library
     * - Handle USB Host events while APP pin in in HIGH state
     */
    task_created = xTaskCreatePinnedToCore(usb_lib_task,
                                           "usb_events",
                                           4096,
                                           xTaskGetCurrentTaskHandle(),
                                           2, NULL, 0);
    assert(task_created == pdTRUE);

    // Wait for notification from usb_lib_task to proceed
    ulTaskNotifyTake(false, 1000);

    task_created = xTaskCreatePinnedToCore(usb_discovery_task,
                                           "usb_discovery",
                                           USB_DISCOVERY_TASK_STACK_SIZE,
                                           NULL,
                                           USB_DISCOVERY_TASK_PRIORITY,
                                           NULL,
                                           0);
    assert(task_created == pdTRUE);

    /*
     * HID host driver configuration
     * - create background task for handling low level event inside the HID driver
     * - provide the device callback to get new HID Device connection event
     */
    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = NULL};

    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    vTaskDelay(1000);
}
