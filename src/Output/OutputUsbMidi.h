#pragma once
#include "IMidiOutput.h"
#include <atomic>

// USB MIDI output class using TinyUSB CDC + MIDI composite device
class OutputUsbMidi : public IMidiOutput {
public:
    OutputUsbMidi() : initialized(false) {}

    void begin() override;
    void end() override;
    void update() override;

    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override;
    void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override;
    void pitchBend(int16_t pitchBend, uint8_t channel = 0) override;

    bool isAvailable() const override;
    const char* getName() const override { return "USB MIDI"; }

private:
    std::atomic<bool> initialized;
};
