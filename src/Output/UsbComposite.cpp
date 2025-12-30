#include "UsbComposite.h"
#include "UsbDescriptors.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include <esp_log.h>

static const char* TAG = "UsbComposite";

bool UsbComposite::initialized = false;

bool UsbComposite::begin() {
    if (initialized) {
        ESP_LOGW(TAG, "USB Composite already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing USB Composite Device (CDC + MIDI)");

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,  // Use default device descriptor
        .string_descriptor = usb_string_descriptor,
        .string_descriptor_count = sizeof(usb_string_descriptor) / sizeof(usb_string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = usb_configuration_descriptor,
        .self_powered = false,
        .vbus_monitor_io = -1,
    };

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
        return false;
    }

    // Initialize CDC ACM
    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = NULL,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL,
    };

    ret = tusb_cdc_acm_init(&acm_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize CDC ACM: %s (MIDI will still work)", esp_err_to_name(ret));
        // Continue even if CDC init fails - MIDI can still work
    }

    initialized = true;
    ESP_LOGI(TAG, "USB Composite Device initialized successfully");
    return true;
}

bool UsbComposite::isConnected() {
    if (!initialized) return false;
    return tud_connected();
}

bool UsbComposite::isMidiMounted() {
    if (!initialized) return false;
    return tud_midi_mounted();
}

bool UsbComposite::isCdcConnected() {
    if (!initialized) return false;
    return tud_cdc_connected();
}
