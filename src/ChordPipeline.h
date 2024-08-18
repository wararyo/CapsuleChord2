#pragma once
#include <cstdint>
#include <vector>
#include "Chord.h"

// キーを押してからMIDIを出力するまでの一連の動作を管理するクラス
class ChordPipeline {
public:
    void sendNotes(bool isNoteOn, std::vector<uint8_t> notes, int vel);
    void playChord(Chord chord);

    std::vector<uint8_t> playingNotes;
};

extern ChordPipeline Pipeline;
