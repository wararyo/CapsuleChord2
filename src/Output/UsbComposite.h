#pragma once

#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// USB Composite Device manager for CDC + MIDI
class UsbComposite {
public:
    // Initialize the USB composite device (CDC + MIDI)
    // Should be called early in setup() to enable USB CDC logging
    // Returns true on success, false on failure
    static bool begin();

    // Redirect ESP_LOG output to USB CDC
    // Returns true on success, false on failure
    static bool initConsole();

    // Restore ESP_LOG output to default (UART)
    // Returns true on success, false on failure
    static bool deinitConsole();

    // Check if USB is connected to host
    static bool isConnected();

    // Check if MIDI interface is mounted
    static bool isMidiMounted();

    // Check if CDC interface is connected
    static bool isCdcConnected();

    // Check if console is redirected to USB CDC
    static bool isConsoleRedirected() { return consoleRedirected; }

    // Start background task to drain MIDI input buffer
    // Prevents host (DAW) from blocking when device is connected
    static void startMidiDrainTask();

    // Stop the MIDI drain background task
    static void stopMidiDrainTask();

private:
    static void midiDrainTask(void* arg);

    static bool initialized;
    static bool cdcInitialized;   // CDC ACM initialization status
    static bool consoleRedirected;
    static TaskHandle_t midiDrainTaskHandle;
    static volatile bool midiDrainTaskRunning;  // Graceful shutdown flag

    // MIDI drain task configuration
    static constexpr uint32_t MIDI_DRAIN_TASK_STACK_SIZE = 2048;  // Minimal stack for simple polling
    static constexpr uint32_t MIDI_DRAIN_POLL_INTERVAL_MS = 1;    // Fast polling to prevent host buffer overflow
    static constexpr uint32_t MIDI_DRAIN_TASK_PRIORITY = 1;       // Low priority background task
    static constexpr int MIDI_DRAIN_TASK_CORE = 1;                // Run on CPU1 (same as TinyUSB task)
};
