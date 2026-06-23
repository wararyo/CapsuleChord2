#include "ChordKeyInput.h"
#include "Keypad.h"
#include "Modifier.h"
#include "SettingsStore.h"

namespace {
constexpr uint8_t NO_DIATONIC_KEY = 0xFF;

const uint8_t kantanNumberKeyMap[] = {
    NO_DIATONIC_KEY, // Custom1
    0,               // Existing Kantan mapping
    NO_DIATONIC_KEY, // Custom2
    3,
    4,
    5,
    0,
    1,
    2
};

bool isLeftKey(uint8_t keyCode) {
    return (keyCode & 0xF0) == 0x00;
}

uint8_t leftButton(uint8_t keyCode) {
    return keyCode & 0x0F;
}

bool getKantanNumber(uint8_t keyCode, uint8_t& number) {
    if (!isLeftKey(keyCode)) return false;
    uint8_t button = leftButton(keyCode);
    if (button < 1 || button > 9) return false;
    number = kantanNumberKeyMap[button - 1];
    return number != NO_DIATONIC_KEY && number <= 6;
}

uint8_t normalizeDegreeRoot(int root) {
    while (root < 0) root += 12;
    while (root >= 12) root -= 12;
    return static_cast<uint8_t>(root);
}

DegreeChord chordToDegreeChord(const Chord& chord, const Scale& scale) {
    DegreeChord degree(
        normalizeDegreeRoot(static_cast<int>(chord.root) - static_cast<int>(scale.key)),
        chord.option,
        chord.inversion
    );
    if (chord.bass != Chord::BASS_DEFAULT) {
        degree.setBass(normalizeDegreeRoot(static_cast<int>(chord.bass) - static_cast<int>(scale.key)));
    }
    return degree;
}

void applyCurrentModifiers(Chord& chord) {
    if (Keypad[KEY_RIGHT_8].isPressed()) thirdInvert(&chord);
    if (Keypad[KEY_RIGHT_7].isPressed()) fifthFlat(&chord);
    if (Keypad[KEY_RIGHT_6].isPressed()) augment(&chord);
    if (Keypad[KEY_RIGHT_9].isPressed()) sus4(&chord);
    if (Keypad[KEY_RIGHT_4].isPressed()) seventhInvert(&chord);
    if (Keypad[KEY_RIGHT_2].isPressed()) ninth(&chord);
    if (Keypad[KEY_RIGHT_1].isPressed()) thirteenth(&chord);
    if (Keypad[KEY_R].isPressed()) pitchUp(&chord);
    if (Keypad[KEY_L].isPressed()) pitchDown(&chord);
    if (Keypad[KEY_RIGHT_3].isPressed()) blackAdder(&chord);
}

void applyRealtimeSeventhPolicy(Chord& chord, const CustomKeyAssignment& assignment) {
    if (!Keypad[KEY_RIGHT_5].isPressed()) return;

    switch (assignment.seventhPolicy) {
        case CustomKeyAssignment::SeventhPolicy::MajorSeventh:
            chord.option &= ~Chord::Seventh;
            chord.option |= Chord::MajorSeventh;
            break;
        case CustomKeyAssignment::SeventhPolicy::DiatonicSeventh:
            // TODO: use scale/degree-aware seventh selection when realtime modifiers are enabled.
            chord.option |= Chord::Seventh;
            break;
        case CustomKeyAssignment::SeventhPolicy::DominantSeventh:
        default:
            chord.option &= ~Chord::MajorSeventh;
            chord.option |= Chord::Seventh;
            break;
    }
}
}  // namespace

bool ChordKeyInput::isCustomKey1(uint8_t keyCode) {
    return keyCode == KEY_LEFT_1;
}

bool ChordKeyInput::isCustomKey2(uint8_t keyCode) {
    return keyCode == KEY_LEFT_3;
}

bool ChordKeyInput::isKantanDiatonicKey(uint8_t keyCode) {
    uint8_t number = 0;
    return getKantanNumber(keyCode, number);
}

DegreeChordInputResult ChordKeyInput::buildDegreeChordFromKantanKey(uint8_t keyCode) {
    uint8_t number = 0;
    if (!getKantanNumber(keyCode, number)) {
        return {};
    }

    Scale scale = Settings.performance.scale.get();
    Chord chord = scale.getDiatonic(number, Keypad[KEY_RIGHT_5].isPressed());
    applyCurrentModifiers(chord);

    DegreeChordInputResult result;
    result.valid = true;
    result.degreeChord = chordToDegreeChord(chord, scale);
    return result;
}

Chord ChordKeyInput::buildPlayableChord(const CustomKeyAssignment& assignment,
                                        bool applyRealtimeModifiers) {
    Scale scale = Settings.performance.scale.get();
    int centerNoteNo = Settings.voicing.centerNoteNo.get();

    Chord chord = scale.degreeToChord(assignment.chord);
    chord.calcInversion(static_cast<uint8_t>(centerNoteNo));

    if (applyRealtimeModifiers) {
        applyRealtimeSeventhPolicy(chord, assignment);
        applyCurrentModifiers(chord);
        if (Keypad[KEY_RT].isPressed()) inversionUp(&chord);
        if (Keypad[KEY_LT].isPressed()) inversionDown(&chord);
    }

    return chord;
}
