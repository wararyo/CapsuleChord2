#pragma once
#include "IMidiOutput.h"

// TODO: ESP32-S3用のUSB MIDIライブラリを追加した後、実装を完成させる
// 現時点ではスタブ実装として、USB MIDIは無効化

// USB MIDI出力クラス（スタブ実装）
class OutputUsbMidi : public IMidiOutput {
public:
    void begin() {}
    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {}
    void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {}
    void pitchBend(int16_t pitchBend, uint8_t channel = 0) override {}
    bool isAvailable() const override { return false; }
    const char* getName() const override { return "USB MIDI (未実装)"; }
};
