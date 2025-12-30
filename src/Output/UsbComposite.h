#pragma once

#include <cstdint>

// USB Composite Device manager for CDC + MIDI
class UsbComposite {
public:
    // Initialize the USB composite device (CDC + MIDI)
    // Returns true on success, false on failure
    static bool begin();

    // Check if USB is connected to host
    static bool isConnected();

    // Check if MIDI interface is mounted
    static bool isMidiMounted();

    // Check if CDC interface is connected
    static bool isCdcConnected();

private:
    static bool initialized;
};
