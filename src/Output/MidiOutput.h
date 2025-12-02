#pragma once
#include "IMidiOutput.h"
#include "OutputInternal.h"
#include "OutputBleMidi.h"
#include "OutputUsbMidi.h"

// 出力デバイスタイプ
enum class OutputDeviceType {
    Internal = 0,   // 内蔵音源
    BleMidi,        // BLE MIDI
    UsbMidi,        // USB MIDI
    Count           // デバイス数
};

// 複数の出力デバイスを管理するクラス
class MidiOutput {
public:
    OutputInternal Internal;    // 内蔵音源
    OutputBleMidi BleMidi;      // BLE MIDI
    OutputUsbMidi UsbMidi;      // USB MIDI

    MidiOutput() : currentDevice(&Internal) {}

    // 現在の出力デバイスを取得
    IMidiOutput* getCurrentDevice() { return currentDevice; }

    // 出力デバイスを切り替え
    void setCurrentDevice(OutputDeviceType type) {
        if (currentDeviceType == type) return;  // 同じデバイスなら何もしない

        // 現在のデバイスを停止
        if (currentDevice) {
            currentDevice->end();
        }

        currentDeviceType = type;
        switch (type) {
            case OutputDeviceType::Internal:
                currentDevice = &Internal;
                break;
            case OutputDeviceType::BleMidi:
                currentDevice = &BleMidi;
                break;
            case OutputDeviceType::UsbMidi:
                currentDevice = &UsbMidi;
                break;
            default:
                currentDevice = &Internal;
                break;
        }

        // 新しいデバイスを開始
        currentDevice->begin();
    }

    // 現在の出力デバイスタイプを取得
    OutputDeviceType getCurrentDeviceType() const { return currentDeviceType; }

    // 指定したタイプのデバイスを取得
    IMidiOutput* getDevice(OutputDeviceType type) {
        switch (type) {
            case OutputDeviceType::Internal:
                return &Internal;
            case OutputDeviceType::BleMidi:
                return &BleMidi;
            case OutputDeviceType::UsbMidi:
                return &UsbMidi;
            default:
                return &Internal;
        }
    }

private:
    IMidiOutput* currentDevice;
    OutputDeviceType currentDeviceType = OutputDeviceType::Internal;
};

extern MidiOutput Output;
