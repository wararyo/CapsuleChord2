#pragma once

#include "AppBase.h"
#include "Tempo.h"
#include "ChordPipeline.h"
#include "Assets/Icons.h"

// Forward declaration
class AppManager;
extern AppManager App;

class AppMetronome : public AppBase
{
public:
    char *getAppName() { return "メトロノーム"; }
    lv_img_dsc_t *getIcon() override { return (lv_img_dsc_t *)&app_metronome; }
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
    class MetronomeSoundTempoCallback : public TempoController::TempoCallbacks
    {
    public:
        AppMetronome *app;
        void onPlayingStateChanged(bool isPlaying) override
        {
        }
        void onTempoChanged(TempoController::tempo_t tempo) override
        {
        }
        void onTick(const TempoController::TickInfo &info) override;
        TempoController::tick_timing_t getTimingMask() override
        {
            return TempoController::TICK_TIMING_BAR | TempoController::TICK_TIMING_FULL;
        }
    };
    class MetronomeUiTempoCallback : public TempoController::TempoCallbacks
    {
    public:
        AppMetronome *app;
        void onPlayingStateChanged(bool isPlaying) override
        {
        }
        void onTempoChanged(TempoController::tempo_t tempo) override
        {
            if (app) app->needsUiUpdate = true;
        }
        void onTick(const TempoController::TickInfo &info) override
        {
        }
        TempoController::tick_timing_t getTimingMask() override
        {
            return 0;
        }
    };
    MetronomeSoundTempoCallback soundTempoCallbacks;
    MetronomeUiTempoCallback uiTempoCallbacks;
    bool isActive = false;
    bool isShowingGui = false;
    bool needsUiUpdate = false;
    lv_obj_t *titleLabel;
    lv_obj_t *switchButton;
    lv_obj_t *tempoContainer;
    lv_obj_t *tempo_label;

    void update();
};
