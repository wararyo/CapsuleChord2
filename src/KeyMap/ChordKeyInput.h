#pragma once

#include <stdint.h>
#include "Chord.h"
#include "CustomKeySettings.h"
#include "Scale.h"

struct DegreeChordInputResult {
    bool valid = false;
    DegreeChord degreeChord;
};

class ChordKeyInput {
public:
    static bool isCustomKey1(uint8_t keyCode);
    static bool isCustomKey2(uint8_t keyCode);
    static bool isKantanDiatonicKey(uint8_t keyCode);

    static DegreeChordInputResult buildDegreeChordFromKantanKey(uint8_t keyCode);
    static Chord buildPlayableChord(const CustomKeyAssignment& assignment,
                                    bool applyRealtimeModifiers = false);
};
