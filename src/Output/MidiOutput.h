#pragma once
#include "IMidiOutput.h"
#include "OutputInternal.h"
#include "OutputBleMidi.h"
#include "OutputUsbMidi.h"

// 出力タイプ
enum class OutputType {
    Internal = 0,   // 内蔵音源
    BleMidi,        // BLE MIDI
    UsbMidi,        // USB MIDI
    Count           // 出力先の数
};

// 複数の出力を管理するクラス
class MidiOutput {
public:
    OutputInternal Internal;    // 内蔵音源
    OutputBleMidi BleMidi;      // BLE MIDI
    OutputUsbMidi UsbMidi;      // USB MIDI

    MidiOutput() : currentOutput(&Internal) {}

    // 現在の出力を取得
    IMidiOutput* getCurrentOutput() { return currentOutput; }

    // 出力を切り替え
    void setCurrentOutput(OutputType type) {
        if (currentOutputType == type) return;  // 同じ出力先なら何もしない

        // 現在の出力先を停止
        if (currentOutput) {
            currentOutput->end();
        }

        currentOutputType = type;
        switch (type) {
            case OutputType::Internal:
                currentOutput = &Internal;
                break;
            case OutputType::BleMidi:
                currentOutput = &BleMidi;
                break;
            case OutputType::UsbMidi:
                currentOutput = &UsbMidi;
                break;
            default:
                currentOutput = &Internal;
                break;
        }

        // 新しい出力先を開始
        currentOutput->begin();
    }

    // 現在の出力タイプを取得
    OutputType getCurrentOutputType() const { return currentOutputType; }

    // 指定したタイプの出力を取得
    IMidiOutput* getOutput(OutputType type) {
        switch (type) {
            case OutputType::Internal:
                return &Internal;
            case OutputType::BleMidi:
                return &BleMidi;
            case OutputType::UsbMidi:
                return &UsbMidi;
            default:
                return &Internal;
        }
    }

private:
    IMidiOutput* currentOutput;
    OutputType currentOutputType = OutputType::Internal;
};

extern MidiOutput Output;
