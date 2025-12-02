#pragma once
#include <cstdint>
#include <vector>
#include <list>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Chord.h"

// キーを押してからMIDIを出力するまでの一連の動作を管理するクラス
class ChordPipeline
{
public:
    ChordPipeline() {
        filterMutex = xSemaphoreCreateMutex();
        if (!filterMutex) {
            Serial.println("FATAL: Failed to create ChordPipeline filterMutex");
        }
    }

    ~ChordPipeline() {
        if (filterMutex) {
            vSemaphoreDelete(filterMutex);
            filterMutex = nullptr;
        }
    }

    // コピー・ムーブ禁止（mutexの二重解放を防ぐ）
    ChordPipeline(const ChordPipeline&) = delete;
    ChordPipeline& operator=(const ChordPipeline&) = delete;
    ChordPipeline(ChordPipeline&&) = delete;
    ChordPipeline& operator=(ChordPipeline&&) = delete;

    // 鳴っているコードの変更を受け取ったり変更するためのフィルター
    class ChordFilter
    {
    public:
        // ChordPipeline外からは使用しない
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

    // 鳴っているノートの変更を受け取ったり変更するためのフィルター
    class NoteFilter
    {
    public:
        // ChordPipeline外からは使用しない
        void setHandler(std::list<NoteFilter *>::iterator handler)
        {
            this->handler = handler;
        }
        std::list<NoteFilter *>::iterator getHandler()
        {
            return handler;
        }
        // このフィルターがノートを変更しうるかどうか
        // trueにする場合はフィルター側でsendNoteを呼び出す、その際にfilterを指定する
        // falseにする場合はsendNoteを呼び出さないこと！(二重に音が鳴る)
        virtual bool modifiesNote() { return false; }
        // ノートが鳴り始めたときに呼び出される
        virtual void onNoteOn(uint8_t note, uint8_t vel, uint8_t channel) = 0;
        virtual void onNoteOff(uint8_t note, uint8_t vel, uint8_t channel) = 0;
    private:
        std::list<NoteFilter *>::iterator handler;
    };

    void sendNote(bool isNoteOn, uint8_t note, uint8_t vel, uint8_t channel = 0, NoteFilter *filter = nullptr);
    void sendNotes(bool isNoteOn, std::vector<uint8_t> notes, uint8_t vel, uint8_t channel = 0, NoteFilter *filter = nullptr);
    void playChord(Chord chord, ChordFilter *filter = nullptr);
    void stopChord(ChordFilter *filter = nullptr);
    void addChordFilter(ChordFilter *filter)
    {
        if (filterMutex) xSemaphoreTake(filterMutex, portMAX_DELAY);
        filter->setHandler(chordFilters.insert(chordFilters.end(), filter));
        if (filterMutex) xSemaphoreGive(filterMutex);
    }
    void removeChordFilter(ChordFilter *filter)
    {
        if (filterMutex) xSemaphoreTake(filterMutex, portMAX_DELAY);
        chordFilters.remove(filter);
        if (filterMutex) xSemaphoreGive(filterMutex);
    }
    void addNoteFilter(NoteFilter *filter)
    {
        if (filterMutex) xSemaphoreTake(filterMutex, portMAX_DELAY);
        filter->setHandler(noteFilters.insert(noteFilters.end(), filter));
        if (filterMutex) xSemaphoreGive(filterMutex);
    }
    void removeNoteFilter(NoteFilter *filter)
    {
        if (filterMutex) xSemaphoreTake(filterMutex, portMAX_DELAY);
        noteFilters.remove(filter);
        if (filterMutex) xSemaphoreGive(filterMutex);
    }

private:
    std::list<ChordFilter *> chordFilters;
    std::list<NoteFilter *> noteFilters;
    std::vector<uint8_t> playingNotes;
    SemaphoreHandle_t filterMutex = nullptr;

    void sendNoteOnRaw(uint8_t note, uint8_t vel, uint8_t channel);
    void sendNoteOffRaw(uint8_t note, uint8_t vel, uint8_t channel);
};

extern ChordPipeline Pipeline;
