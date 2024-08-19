#pragma once

#include <lvgl.h>
#include "Tempo.h"

class TempoDialog
{
public:
    void create();
    void del();
    void update();

private:
    class TempoControllerCallbacks : public TempoController::TempoCallbacks
    {
    public:
        TempoDialog *dialog;
        void onTempoChanged(TempoController::tempo_t tempo) override
        {
            dialog->update();
        }
        void onTick(TempoController::tick_timing_t timing) override
        {
        }
    };
    TempoControllerCallbacks tempoControllerCallbacks;
    lv_obj_t *bg = nullptr;
    lv_obj_t *frame = nullptr;
    lv_obj_t *tempo_label = nullptr;
    lv_obj_t *tempo_button_plus = nullptr;
    lv_obj_t *tempo_button_minus = nullptr;
    lv_obj_t *button_play = nullptr;
};
