#include "ChordKeyInput.h"
#include "Keypad.h"
#include "Modifier.h"
#include "Scale.h"
#include "SettingsStore.h"

namespace {
// 右キーパッド等の修飾キー状態をDegreeChordに反映する
void applyCurrentModifiers(DegreeChord& degree) {
    if (Keypad[KEY_RIGHT_8].isPressed()) thirdInvert(&degree);
    if (Keypad[KEY_RIGHT_7].isPressed()) fifthFlat(&degree);
    if (Keypad[KEY_RIGHT_6].isPressed()) augment(&degree);
    if (Keypad[KEY_RIGHT_9].isPressed()) sus4(&degree);
    if (Keypad[KEY_RIGHT_4].isPressed()) seventhInvert(&degree);
    if (Keypad[KEY_RIGHT_2].isPressed()) ninth(&degree);
    if (Keypad[KEY_RIGHT_1].isPressed()) thirteenth(&degree);
    if (Keypad[KEY_R].isPressed())       pitchUp(&degree);
    if (Keypad[KEY_L].isPressed())       pitchDown(&degree);
    if (Keypad[KEY_RIGHT_3].isPressed()) blackAdder(&degree);
}
}  // namespace

KeyInputResult ChordKeyInput::resolveKey(uint8_t keyCode, const uint8_t* numberKeyMap) {
    KeyInputResult result;
    if ((keyCode & 0xF0) != 0x00) return result;  // 左キーパッド以外
    uint8_t button = keyCode & 0x0F;
    if (button < 1 || button > 9) return result;

    uint8_t number = numberKeyMap[button - 1];  // Key number starts from 1
    if (number == KEYNUM_CUSTOM1) {
        result.kind = KeyInputResult::Kind::Custom1;
        return result;
    }
    if (number == KEYNUM_CUSTOM2) {
        result.kind = KeyInputResult::Kind::Custom2;
        return result;
    }
    if (number > 6) return result;  // KEYNUM_NONEなど

    Scale scale = Settings.performance.scale.get();  // コピーを取得
    DegreeChord degree = scale.getDiatonicDegree(number, Keypad[KEY_RIGHT_5].isPressed());
    applyCurrentModifiers(degree);

    result.kind = KeyInputResult::Kind::Degree;
    result.degree = degree;
    return result;
}
