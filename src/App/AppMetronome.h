#pragma once

#include "AppBase.h"
#include "Tempo.h"
#include "ChordPipeline.h"

class AppMetronome : public AppBase
{
public:
    char *getAppName() { return "メトロノーム"; }
    bool runsInBackground() { return true; }

    void onCreate() override;
    void onActivate() override;
    void onDeactivate() override;
    void onShowGui(lv_obj_t *container) override;
    void onHideGui() override;
    void onDestroy() override;
private:
    class MetronomeTempoCallback : public TempoController::TempoCallbacks
    {
        void onTempoChanged(TempoController::tempo_t tempo) override
        {
        }
        void onTick(TempoController::tick_timing_t timing) override
        {
            Pipeline.sendNotes(true, {timing & TempoController::TICK_TIMING_BAR ? 25 : 24}, 64, 0xF);
        }
        TempoController::tick_timing_t getTimingMask() override
        {
            return TempoController::TICK_TIMING_BAR | TempoController::TICK_TIMING_FULL;
        }
    };
    MetronomeTempoCallback tempoCallbacks;
    bool isActive = false;
    lv_obj_t *button;
    lv_obj_t *label;
};
