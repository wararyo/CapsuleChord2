#pragma once

#include <stdint.h>
#include "Archive.h"
#include "Chord.h"

struct CustomKeyAssignment {
    enum class SeventhPolicy : uint8_t {
        DominantSeventh = 0,
        MajorSeventh = 1,
        DiatonicSeventh = 2
    };

    DegreeChord chord;
    SeventhPolicy seventhPolicy;

    CustomKeyAssignment()
        : chord(DegreeChord::IISharp, 0), seventhPolicy(SeventhPolicy::DominantSeventh) {}

    CustomKeyAssignment(DegreeChord chord, SeventhPolicy seventhPolicy)
        : chord(chord), seventhPolicy(seventhPolicy) {}

    bool operator==(const CustomKeyAssignment& other) const {
        return chord.root == other.chord.root &&
               chord.option == other.chord.option &&
               chord.inversion == other.chord.inversion &&
               chord.bass == other.chord.bass &&
               seventhPolicy == other.seventhPolicy;
    }

    bool operator!=(const CustomKeyAssignment& other) const {
        return !(*this == other);
    }

    void serialize(OutputArchive& archive, const char* key) const {
        archive.pushNest(key);
        DegreeChord chordCopy = chord;  // DegreeChord::serialize is non-const
        archive("chord", chordCopy);
        archive("seventhPolicy", static_cast<uint8_t>(seventhPolicy));
        archive.popNest();
    }

    void deserialize(InputArchive& archive, const char* key) {
        if (!archive.pushNest(key)) return;

        archive("chord", chord);

        uint8_t rawPolicy = static_cast<uint8_t>(seventhPolicy);
        archive("seventhPolicy", rawPolicy);
        if (rawPolicy <= static_cast<uint8_t>(SeventhPolicy::DiatonicSeventh)) {
            seventhPolicy = static_cast<SeventhPolicy>(rawPolicy);
        }

        archive.popNest();
    }
};
