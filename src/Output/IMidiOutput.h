#pragma once
#include <cstdint>

// すべてのMIDI出力デバイスが実装すべき抽象インターフェース
class IMidiOutput {
public:
    virtual ~IMidiOutput() = default;

    // 出力デバイスを開始（この出力に切り替えた時に呼ばれる）
    virtual void begin() {}

    // 出力デバイスを停止（別の出力に切り替える時に呼ばれる）
    virtual void end() {}

    // ノートオン
    virtual void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) = 0;

    // ノートオフ
    virtual void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) = 0;

    // ピッチベンド
    virtual void pitchBend(int16_t pitchBend, uint8_t channel = 0) = 0;

    // 出力デバイスが利用可能かどうか
    virtual bool isAvailable() const = 0;

    // 出力デバイスの名前
    virtual const char* getName() const = 0;
};
