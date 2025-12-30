#include "OutputUsbMidi.h"
#include "UsbComposite.h"
#include "class/midi/midi_device.h"
#include <esp_log.h>

static const char* TAG = "OutputUsbMidi";

void OutputUsbMidi::begin() {
    if (initialized) {
        ESP_LOGW(TAG, "USB MIDI already initialized");
        return;
    }

    ESP_LOGI(TAG, "Initializing USB MIDI output...");

    if (!UsbComposite::begin()) {
        ESP_LOGE(TAG, "Failed to initialize USB Composite device");
        return;
    }

    initialized = true;
    ESP_LOGI(TAG, "USB MIDI output ready");
}

void OutputUsbMidi::end() {
    if (!initialized) return;

    initialized = false;
    ESP_LOGI(TAG, "USB MIDI output stopped");
}

void OutputUsbMidi::update() {
    // Connection state is checked in isAvailable()
    // No periodic update needed
}

void OutputUsbMidi::noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    if (!initialized || !UsbComposite::isMidiMounted()) return;

    uint8_t msg[3] = {
        static_cast<uint8_t>(0x90 | (channel & 0x0F)),  // Note On + channel
        static_cast<uint8_t>(note & 0x7F),              // Note number
        static_cast<uint8_t>(velocity & 0x7F)           // Velocity
    };
    tud_midi_stream_write(0, msg, 3);
}

void OutputUsbMidi::noteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
    if (!initialized || !UsbComposite::isMidiMounted()) return;

    uint8_t msg[3] = {
        static_cast<uint8_t>(0x80 | (channel & 0x0F)),  // Note Off + channel
        static_cast<uint8_t>(note & 0x7F),              // Note number
        static_cast<uint8_t>(velocity & 0x7F)           // Velocity
    };
    tud_midi_stream_write(0, msg, 3);
}

void OutputUsbMidi::pitchBend(int16_t pitchBend, uint8_t channel) {
    if (!initialized || !UsbComposite::isMidiMounted()) return;

    // Convert from -8192..8191 to 0..16383 (14-bit value)
    uint16_t value = static_cast<uint16_t>(pitchBend + 8192);
    uint8_t msg[3] = {
        static_cast<uint8_t>(0xE0 | (channel & 0x0F)),  // Pitch Bend + channel
        static_cast<uint8_t>(value & 0x7F),             // LSB (7 bits)
        static_cast<uint8_t>((value >> 7) & 0x7F)       // MSB (7 bits)
    };
    tud_midi_stream_write(0, msg, 3);
}

bool OutputUsbMidi::isAvailable() const {
    return initialized && UsbComposite::isMidiMounted();
}
