#include "UsbComposite.h"
#include "UsbDescriptors.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "class/midi/midi_device.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "UsbComposite";

bool UsbComposite::initialized = false;
bool UsbComposite::consoleRedirected = false;
TaskHandle_t UsbComposite::midiDrainTaskHandle = nullptr;
volatile bool UsbComposite::midiDrainTaskRunning = false;

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

bool UsbComposite::initConsole() {
    if (!initialized) {
        ESP_LOGW(TAG, "USB Composite not initialized, cannot redirect console");
        return false;
    }

    if (consoleRedirected) {
        return true;  // Already redirected
    }

    esp_err_t ret = esp_tusb_init_console(TINYUSB_CDC_ACM_0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to redirect console to USB CDC: %s", esp_err_to_name(ret));
        return false;
    }

    consoleRedirected = true;
    ESP_LOGI(TAG, "Console redirected to USB CDC");
    return true;
}

bool UsbComposite::deinitConsole() {
    if (!consoleRedirected) {
        return true;  // Already using default output
    }

    esp_err_t ret = esp_tusb_deinit_console(TINYUSB_CDC_ACM_0);
    if (ret != ESP_OK) {
        // Cannot log to ESP_LOG here as console state is uncertain
        return false;
    }

    consoleRedirected = false;
    ESP_LOGI(TAG, "Console restored to UART");
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

void UsbComposite::midiDrainTask(void* arg) {
    uint8_t packet[4];
    while (midiDrainTaskRunning) {
        vTaskDelay(pdMS_TO_TICKS(MIDI_DRAIN_POLL_INTERVAL_MS));
        if (!initialized || !tud_midi_mounted()) continue;

        while (tud_midi_available()) {
            tud_midi_packet_read(packet);
            // Discard received MIDI data - we don't process MIDI input
        }
    }

    // Self-delete when shutdown flag is set
    vTaskDelete(nullptr);
}

void UsbComposite::startMidiDrainTask() {
    if (midiDrainTaskHandle != nullptr) {
        ESP_LOGW(TAG, "MIDI drain task already running");
        return;
    }

    midiDrainTaskRunning = true;
    xTaskCreatePinnedToCore(
        midiDrainTask,
        "midi_drain",
        MIDI_DRAIN_TASK_STACK_SIZE,
        nullptr,
        MIDI_DRAIN_TASK_PRIORITY,
        &midiDrainTaskHandle,
        MIDI_DRAIN_TASK_CORE
    );
    ESP_LOGI(TAG, "MIDI drain task started");
}

void UsbComposite::stopMidiDrainTask() {
    if (midiDrainTaskHandle == nullptr) return;

    // Signal task to stop
    midiDrainTaskRunning = false;

    // Wait for task to self-delete (max 200ms)
    ESP_LOGI(TAG, "Waiting for MIDI drain task to stop...");
    for (int i = 0; i < 20 && eTaskGetState(midiDrainTaskHandle) != eDeleted; i++) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Force delete if task didn't stop gracefully
    if (eTaskGetState(midiDrainTaskHandle) != eDeleted) {
        ESP_LOGW(TAG, "MIDI drain task did not stop gracefully, forcing delete");
        vTaskDelete(midiDrainTaskHandle);
    }

    midiDrainTaskHandle = nullptr;
    ESP_LOGI(TAG, "MIDI drain task stopped");
}
