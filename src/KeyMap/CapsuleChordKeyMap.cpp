#include "CapsuleChordKeyMap.h"
#include "Modifier.h"
#include "BLEMidi.h"
#include "ChordPipeline.h"
#include "Scale.h"
#include "SettingsStore.h"
#include "KeyMap/ChordKeyInput.h"

const uint8_t CapsuleChordKeyMap::numberKeyMap[] = {
    6,               //VII
    KEYNUM_CUSTOM1,  //Custom1
    KEYNUM_CUSTOM2,  //Custom2
    3, //IV
    0, //I
    4, //V
    1, //II
    5, //VI
    2};

bool CapsuleChordKeyMap::onKeyPressed(uint8_t keyCode) {
  KeyInputResult result = ChordKeyInput::resolveKey(keyCode, numberKeyMap);
  if (result.kind == KeyInputResult::Kind::None) return false; // イベントを消費しない

  DegreeChord degree;
  switch (result.kind) {
    case KeyInputResult::Kind::Custom1:
      degree = Settings.controls.customKey1.get().chord;
      break;
    case KeyInputResult::Kind::Custom2:
      degree = Settings.controls.customKey2.get().chord;
      break;
    default:
      degree = result.degree;
      break;
  }

  Scale scale = Settings.performance.scale.get();  // コピーを取得
  int centerNoteNo = Settings.voicing.centerNoteNo.get();
  Chord c = scale.realizeChord(degree, (uint8_t)centerNoteNo);
  if (result.kind == KeyInputResult::Kind::Degree) {
    // 転回修飾はカスタムキーには適用しない（保存されたコードをそのまま鳴らす）
    if(Keypad[KEY_RT].isPressed()) inversionUp(&c);
    if(Keypad[KEY_LT].isPressed()) inversionDown(&c);
  }
  Pipeline.playChord(c);
  return true; // イベントを消費
}

bool CapsuleChordKeyMap::onKeyReleased(uint8_t keyCode) {
  if ((keyCode & 0xF0) == 0x00) { // 左キーパッドが離された場合
    Pipeline.stopChord();
    return true; // イベントを消費
  }
  return false; // イベントを消費しない
}
