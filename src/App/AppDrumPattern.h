#pragma once

#include "AppBase.h"
#include "Tempo.h"
#include "ChordPipeline.h"
#include "Assets/Icons.h"

class AppDrumPattern : public AppBase
{
public:
    struct DrumPatternItem
    {
        musical_time_t time;
        uint8_t status;
        uint8_t data1;
        uint8_t data2;
    };
    char *getAppName() { return "ドラム"; }
    lv_img_dsc_t *getIcon() override { return (lv_img_dsc_t *)&app_drum; }
    bool runsInBackground() { return true; }

    bool getActive() { return isActive; }
    void onCreate() override;
    void onActivate() override;
    void onDeactivate() override;
    void onShowGui(lv_obj_t *container) override;
    void onHideGui() override;
    void onDestroy() override;    
    void onUpdateGui() override;

private:
    class TempoCallbacks : public TempoController::TempoCallbacks
    {
    public:
        AppDrumPattern *app;
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
                TempoController::TICK_TIMING_FULL_TRIPLET |
                TempoController::TICK_TIMING_HALF |
                TempoController::TICK_TIMING_HALF_TRIPLET |
                TempoController::TICK_TIMING_QUARTER | 
                TempoController::TICK_TIMING_EIGHTH;
        }
    private:
        void processItem(const DrumPatternItem &item);
        bool shouldKnock = false; // 今回の処理の終わりでノックするかどうか
    };
    TempoCallbacks tempoCallbacks;
    bool isActive = false;
    bool isShowingGui = false;
    musical_time_t previousTime = 0;
    lv_obj_t *titleLabel;
    lv_obj_t *switchButton;
    lv_obj_t *patternContainer;
    lv_obj_t *cursorRect;

    // UI update flags
    bool needsCursorUpdate = false;
    musical_time_t cursorTimeInBar = 0;

    void update();
};
