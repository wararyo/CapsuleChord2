#pragma once
#include <cstdint>
#include <vector>
#include <list>
#include "Chord.h"

// キーを押してからMIDIを出力するまでの一連の動作を管理するクラス
class ChordPipeline
{
public:
    // 鳴っているコードの変更を通知するためのインターフェース
    class PipelineCallbacks
    {
    public:
        virtual void onChordChanged(Chord chord) = 0;
    };

    void sendNote(bool isNoteOn, uint8_t note, uint8_t vel, uint8_t channel = 0);
    void sendNotes(bool isNoteOn, std::vector<uint8_t> notes, uint8_t vel, uint8_t channel = 0);
    void playChord(Chord chord);
    void stopChord();
    void addListener(PipelineCallbacks *listener)
    {
        listeners.push_back(listener);
    }
    void removeListener(PipelineCallbacks *listener)
    {
        listeners.remove(listener);
    }

private:
    std::list<PipelineCallbacks *> listeners;
    std::vector<uint8_t> playingNotes;
};

extern ChordPipeline Pipeline;
