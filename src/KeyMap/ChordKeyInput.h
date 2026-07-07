#pragma once

#include <stdint.h>
#include "Chord.h"

// numberKeyMapに埋め込むセンチネル値。
// 0-6は通常のダイアトニック番号、それ以外はこのセンチネルで役割を示す。
constexpr uint8_t KEYNUM_NONE    = 0xFD;
constexpr uint8_t KEYNUM_CUSTOM1 = 0xFE;
constexpr uint8_t KEYNUM_CUSTOM2 = 0xFF;

struct KeyInputResult {
    enum class Kind : uint8_t { None, Degree, Custom1, Custom2 };
    Kind kind = Kind::None;
    DegreeChord degree;  // kind == Kind::Degree のとき有効（修飾キー適用済み）
};

// キーパッド入力からDegreeChordへの「変換」層。
// 調への具現化（degreeToChord + calcInversion）はScale::realizeChord()が担う。
class ChordKeyInput {
public:
    // 左キーパッドのキー押下を、現在の修飾キー状態を反映したDegreeChordまたは
    // カスタムキー種別に解決する。numberKeyMapは各KeyMapの物理キー→度数番号テーブル（9要素）。
    static KeyInputResult resolveKey(uint8_t keyCode, const uint8_t* numberKeyMap);
};
