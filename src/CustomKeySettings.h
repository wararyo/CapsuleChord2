#pragma once

#include <stdint.h>
#include "Archive.h"
#include "Chord.h"

struct CustomKeyAssignment {
    // セブンスキー押下時に付与する7度の種類。
    // 3度/5度の質は保存されたchord自身が持つため、m7/m7-5/mM7などは
    // トライアドの質とこの2値の組み合わせで自動的に表現される。
    enum class SeventhType : uint8_t {
        Seventh = 0,      // 短7度 (b7)
        MajorSeventh = 1  // 長7度 (M7)
    };

    DegreeChord chord;
    SeventhType seventhType;

    CustomKeyAssignment()
        : chord(DegreeChord::IISharp, 0), seventhType(SeventhType::Seventh) {}

    CustomKeyAssignment(DegreeChord chord, SeventhType seventhType)
        : chord(chord), seventhType(seventhType) {}

    bool operator==(const CustomKeyAssignment& other) const {
        return chord.root == other.chord.root &&
               chord.option == other.chord.option &&
               chord.inversion == other.chord.inversion &&
               chord.bass == other.chord.bass &&
               seventhType == other.seventhType;
    }

    bool operator!=(const CustomKeyAssignment& other) const {
        return !(*this == other);
    }

    void serialize(OutputArchive& archive, const char* key) const {
        archive.pushNest(key);
        DegreeChord chordCopy = chord;  // DegreeChord::serialize is non-const
        archive("chord", chordCopy);
        archive("seventhType", static_cast<uint8_t>(seventhType));
        archive.popNest();
    }

    void deserialize(InputArchive& archive, const char* key) {
        if (!archive.pushNest(key)) return;

        archive("chord", chord);

        uint8_t rawType = static_cast<uint8_t>(seventhType);
        archive("seventhType", rawType);
        if (rawType <= static_cast<uint8_t>(SeventhType::MajorSeventh)) {
            seventhType = static_cast<SeventhType>(rawType);
        }

        archive.popNest();
    }
};

// chordの7度を指定された種類に設定する（既存の7度ビットは置換）
inline void applySeventhType(DegreeChord& chord, CustomKeyAssignment::SeventhType type) {
    chord.option &= ~(Chord::Seventh | Chord::MajorSeventh);
    chord.option |= (type == CustomKeyAssignment::SeventhType::MajorSeventh)
        ? Chord::MajorSeventh : Chord::Seventh;
}
