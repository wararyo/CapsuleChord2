#include "CapsuleChordKeyMap.h"
#include "Modifier.h"
#include "BLEMidi.h"
#include "ChordPipeline.h"
#include "Scale.h"
#include "SettingsStore.h"

const uint8_t CapsuleChordKeyMap::numberKeyMap[] = {
    0, //Custom1
    6, //VII
    0, //Custom2
    3, //IV
    0, //I
    4, //V
    1, //II
    5, //VI
    2};

bool CapsuleChordKeyMap::onKeyPressed(uint8_t keyCode) {
  if ((keyCode & 0xF0) == 0x00) { // 左キーパッドが押された場合
    uint8_t button = keyCode & 0x0F;
    uint8_t number = numberKeyMap[button - 1]; // Key number starts from 1
    if(0 <= number && number <= 6) {
      Scale scale = Settings.performance.scale.get();  // コピーを取得（getDiatonicがnon-constのため）
      int centerNoteNo = Settings.voicing.centerNoteNo.get();
      Chord c = scale.getDiatonic(number,Keypad[KEY_RIGHT_5].isPressed());
      if(Keypad[KEY_RIGHT_8].isPressed())   thirdInvert(&c);
      if(Keypad[KEY_RIGHT_7].isPressed())   fifthFlat(&c);
      if(Keypad[KEY_RIGHT_6].isPressed())   augment(&c);
      if(Keypad[KEY_RIGHT_9].isPressed())   sus4(&c);
      // if(Keypad[KEY_RIGHT_5].isPressed()) thirdInvert(&c);
      if(Keypad[KEY_RIGHT_4].isPressed())   seventhInvert(&c);
      if(Keypad[KEY_RIGHT_2].isPressed())   ninth(&c);
      if(Keypad[KEY_RIGHT_1].isPressed())   thirteenth(&c);
      if(Keypad[KEY_R].isPressed())         pitchUp(&c);
      if(Keypad[KEY_L].isPressed())         pitchDown(&c);
      if(Keypad[KEY_RIGHT_3].isPressed())   blackAdder(&c);
      c.calcInversion((uint8_t)centerNoteNo);
      if(Keypad[KEY_RT].isPressed())        inversionUp(&c);
      if(Keypad[KEY_LT].isPressed())        inversionDown(&c);
      Pipeline.playChord(c);
      return true; // イベントを消費
    }
  }
  else if ((keyCode & 0xF0) == 0x20) { // その他のキーが押された場合
    // 現在はコメントアウトされている
    // if (keyCode == KEY_RT) *(centerNoteNo) += 4;
    // if (keyCode == KEY_LT) *(centerNoteNo) -= 4;
  }
  return false; // イベントを消費しない
}

bool CapsuleChordKeyMap::onKeyReleased(uint8_t keyCode) {
  if ((keyCode & 0xF0) == 0x00) { // 左キーパッドが離された場合
    Pipeline.stopChord();
    return true; // イベントを消費
  }
  else if ((keyCode & 0xF0) == 0x20) { // その他のキーが離された場合
    // 現在はコメントアウトされている
    // if (keyCode == KEY_RT) *(centerNoteNo) -= 4;
    // if (keyCode == KEY_LT) *(centerNoteNo) += 4;
  }
  return false; // イベントを消費しない
}
