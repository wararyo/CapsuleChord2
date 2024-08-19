#pragma once
#include <cstdint>
#include <vector>
#include <list>
#include "Chord.h"

// キーを押してからMIDIを出力するまでの一連の動作を管理するクラス
class ChordPipeline {
public:
    // 鳴っているコードの変更を通知するためのインターフェース
    class PipelineCallbacks {
    public:
        virtual void onChordChanged(Chord chord) = 0;
    };

    void sendNotes(bool isNoteOn, std::vector<uint8_t> notes, int vel, uint8_t channel = 0);
    void playChord(Chord chord);
    void addListener(PipelineCallbacks* listener) {
        listeners.push_back(listener);
    }
    void removeListener(PipelineCallbacks* listener) {
        listeners.remove(listener);
    }

    std::vector<uint8_t> playingNotes;

private:
    std::list<PipelineCallbacks*> listeners;
};

extern ChordPipeline Pipeline;
