#pragma once

#include <list>
#include <memory>

#include "AppBase.h"
#include "Tempo.h"
#include "ChordPipeline.h"

class AppSequencer : public AppBase
{
public:
    // SequenceItem内ではNoteNoの代わりに、コードトーンを相対的に表た値を使う
    // 60を基準とする
    // 同一オクターブ内で値が1増えると、1つ上のコードトーンを示す
    // 値が12増えると、同一コードトーンの一つ上のオクターブを示す
    struct SequenceItem
    {
        musical_time_t time;
        uint8_t status;
        uint8_t data1;
        uint8_t data2;
    };
    char *getAppName() { return "シーケンサー"; }
    bool runsInBackground() { return true; }

    bool getActive() { return isActive; }
    void onCreate() override;
    void onActivate() override;
    void onDeactivate() override;
    void onShowGui(lv_obj_t *container) override;
    void onHideGui() override;
    void onDestroy() override;

    bool isPlayingNotes() { return !input.empty(); }
private:
    class TempoCallbacks : public TempoController::TempoCallbacks
    {
    public:
        AppSequencer *app;
        void onPlayingStateChanged(bool isPlaying) override
        {
            if (isPlaying) app->previousTime = Tempo.getMusicalTime();
        }
        void onTempoChanged(TempoController::tempo_t tempo) override
        {
        }
        void onTick(TempoController::tick_timing_t timing, musical_time_t time) override;
        TempoController::tick_timing_t getTimingMask() override
        {
            return TempoController::TICK_TIMING_BAR |
                TempoController::TICK_TIMING_FULL |
                TempoController::TICK_TIMING_HALF |
                TempoController::TICK_TIMING_QUARTER | 
                TempoController::TICK_TIMING_EIGHTH;
        }
    };
    class NoteFilter : public ChordPipeline::NoteFilter
    {
    public:
        AppSequencer *app;
        bool modifiesNote() { return true; }
        void onNoteOn(uint8_t note, uint8_t vel, uint8_t channel) override;
        void onNoteOff(uint8_t note, uint8_t vel, uint8_t channel) override;
    };
    TempoCallbacks tempoCallbacks;
    NoteFilter noteFilter;
    bool isActive = false;
    musical_time_t previousTime = 0;
    lv_obj_t *titleLabel;
    lv_obj_t *switchButton;
    std::list<uint8_t> inputBuffer; // 入力に来た発音中のノート (最新)
    std::list<uint8_t> input; // 入力に来た発音中のノート (直前のTICK_TIMING_FULLなtick時)
    uint8_t inputProcessed[12] = {60};
    std::list<uint8_t> output; // 出力した発音中のノート
    portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    std::shared_ptr<std::list<SequenceItem>> currentSequence;

    void updateUi();
    void updatePlayingNotes();
    void processItem(const SequenceItem &item);
};
