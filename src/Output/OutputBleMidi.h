#pragma once
#include "IMidiOutput.h"
#include "../BLEMidi.h"
#include <BLEDevice.h>

#define BLE_MIDI_DEVICE_NAME "CapsuleChord2"

// BLE MIDI用のサーバーコールバック
class BleMidiServerCallbacks : public BLEMidiServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override {
        BLEMidiServerCallbacks::onConnect(pServer);
        // 接続時の処理
    }

    void onDisconnect(BLEServer* pServer) override {
        BLEMidiServerCallbacks::onDisconnect(pServer);
        // 切断後、再度アドバタイズを開始
        pServer->startAdvertising();
    }
};

// BLE MIDI出力クラス
class OutputBleMidi : public IMidiOutput {
public:
    OutputBleMidi() : initialized(false) {}

    void begin() override {
        if (!initialized) {
            Midi.begin(BLE_MIDI_DEVICE_NAME, new BleMidiServerCallbacks(), nullptr);
            initialized = true;
        }
    }

    void end() override {
        // if (initialized) {
        //     // アドバタイジングを停止
        //     BLEDevice::stopAdvertising();
        //     // BLEを完全に停止（メモリは解放しない、再初期化を容易にするため）
        //     BLEDevice::deinit(false);
        //     initialized = false;
        // }
    }

    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        // if (initialized && Midi.isConnected) {
        //     // MIDIノートオン: 0x90 + channel
        //     Midi.sendNote(0x90 | (channel & 0x0F), note, velocity);
        // }
    }

    void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        // if (initialized && Midi.isConnected) {
        //     // MIDIノートオフ: 0x80 + channel
        //     Midi.sendNote(0x80 | (channel & 0x0F), note, velocity);
        // }
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
