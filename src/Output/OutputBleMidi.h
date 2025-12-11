#pragma once
#include "IMidiOutput.h"
#include "../BLEMidi.h"

#define BLE_MIDI_DEVICE_NAME "CapsuleChord2"

// BLE MIDI出力クラス
class OutputBleMidi : public IMidiOutput {
public:
    OutputBleMidi() : initialized(false) {}

    void begin() override {
        if (!initialized) {
            Midi.begin(BLE_MIDI_DEVICE_NAME);
            initialized = true;
        }
    }

    void end() override {
        if (initialized) {
            Midi.end();
            initialized = false;
        }
    }

    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        if (initialized && Midi.isConnected) {
            Midi.sendNoteOn(channel, note, velocity);
        }
    }

    void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        if (initialized && Midi.isConnected) {
            Midi.sendNoteOff(channel, note);
        }
    }

    void pitchBend(int16_t pitchBend, uint8_t channel = 0) override {
        // BLE MIDIにはピッチベンドの実装がないため、現時点ではスキップ
    }

    bool isAvailable() const override {
        return initialized && Midi.isConnected;
    }

    const char* getName() const override { return "BLE MIDI"; }

private:
    bool initialized;
};
