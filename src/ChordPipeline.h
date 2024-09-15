#pragma once
#include <cstdint>
#include <vector>
#include <list>
#include "Chord.h"

// キーを押してからMIDIを出力するまでの一連の動作を管理するクラス
class ChordPipeline
{
public:
    // 鳴っているコードの変更を受け取ったり変更するためのフィルター
    class ChordFilter
    {
    public:
        void setHandler(std::list<ChordFilter *>::iterator handler)
        {
            this->handler = handler;
        }
        std::list<ChordFilter *>::iterator getHandler()
        {
            return handler;
        }
        // このフィルターがコードを変更しうるかどうか
        // trueにする場合はフィルター側でplayChordを呼び出す、その際にfilterを指定する
        // falseにする場合はplayChordを呼び出さないこと！(二重にコードが鳴る)
        virtual bool modifiesChord() { return false; }
        // コードが鳴り始めたときに呼び出される
        virtual void onChordOn(Chord chord) = 0;
        virtual void onChordOff() = 0;
    private:
        std::list<ChordFilter *>::iterator handler;
    };

    void sendNote(bool isNoteOn, uint8_t note, uint8_t vel, uint8_t channel = 0);
    void sendNotes(bool isNoteOn, std::vector<uint8_t> notes, uint8_t vel, uint8_t channel = 0);
    void playChord(Chord chord, ChordFilter *filter = nullptr);
    void stopChord(ChordFilter *filter = nullptr);
    void addChordFilter(ChordFilter *filter)
    {
        filter->setHandler(chordFilters.insert(chordFilters.end(), filter));
    }
    void removeChordFilter(ChordFilter *filter)
    {
        chordFilters.remove(filter);
    }

private:
    std::list<ChordFilter *> chordFilters;
    std::vector<uint8_t> playingNotes;
};

extern ChordPipeline Pipeline;
