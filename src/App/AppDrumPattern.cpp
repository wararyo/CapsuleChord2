#include "AppDrumPattern.h"

void AppDrumPattern::update()
{
    // 有効/無効スイッチ
    if (isActive) lv_obj_add_state(switchButton, LV_STATE_CHECKED);
    else lv_obj_clear_flag(switchButton, LV_STATE_CHECKED);
}

void AppDrumPattern::onCreate()
{
    isActive = false;
    tempoCallbacks.app = this;
}

void AppDrumPattern::onActivate()
{
    isActive = true;
    previousTime = Tempo.getMusicalTime();
    Tempo.addListener(&tempoCallbacks);
}

void AppDrumPattern::onDeactivate()
{
    isActive = false;
    Tempo.removeListener(&tempoCallbacks);
}

void AppDrumPattern::onShowGui(lv_obj_t *container)
{
    // アプリタイトル
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 4);

    // 有効/無効スイッチ
    switchButton = lv_switch_create(container);
    lv_obj_set_size(switchButton, 136, 80);
    lv_obj_center(switchButton);
    lv_obj_add_event_cb(switchButton, [](lv_event_t *e) {
        auto *self = (AppDrumPattern *)lv_event_get_user_data(e);
        // ここでisActiveを反転させる
        self->isActive = !self->isActive;
        // 以降のisActiveの値は変更後の値
        if (self->isActive)
        {
            self->previousTime = Tempo.getMusicalTime();
            Tempo.addListener(&self->tempoCallbacks);
        }
        else
        {
            Tempo.removeListener(&self->tempoCallbacks);
        }
        self->update();
    }, LV_EVENT_CLICKED, (void *)this);

    update();
}

void AppDrumPattern::onHideGui()
{
    lv_obj_del(titleLabel);
    lv_obj_del(switchButton);
}

void AppDrumPattern::onDestroy()
{
}

const struct AppDrumPattern::DrumPatternItem pattern[] = {
{0,0x99,0x24,0x7F},
{0,0x99,0x2A,0x47},
{240,0x89,0x2A,0x40},
{240,0x99,0x2A,0x40},
{480,0x89,0x24,0x40},
{480,0x89,0x2A,0x40},
{480,0x99,0x26,0x7F},
{480,0x99,0x2A,0x68},
{720,0x89,0x2A,0x40},
{720,0x99,0x2A,0x3C},
{960,0x89,0x26,0x40},
{960,0x89,0x2A,0x40},
{960,0x99,0x24,0x7F},
{960,0x99,0x2A,0x4F},
{1200,0x89,0x24,0x40},
{1200,0x89,0x2A,0x40},
{1200,0x99,0x24,0x7F},
{1200,0x99,0x2A,0x35},
{1440,0x89,0x24,0x40},
{1440,0x89,0x2A,0x40},
{1440,0x99,0x26,0x7F},
{1440,0x99,0x2A,0x6A},
{1680,0x89,0x2A,0x40},
{1680,0x99,0x2A,0x38},
{1920,0x89,0x26,0x40},
{1920,0x89,0x2A,0x40},
};

void process_item(const AppDrumPattern::DrumPatternItem &item)
{
    if ((item.status & 0xF0) == 0x90)
    {
        Pipeline.sendNote(true, item.data1, item.data2, item.status & 0x0F);
    }
    else if ((item.status & 0xF0) == 0x80)
    {
        Pipeline.sendNote(false, item.data1, item.data2, item.status & 0x0F);
    }
}

void AppDrumPattern::TempoCallbacks::onTick(TempoController::tick_timing_t timing, musical_time_t time)
{
    const musical_time_t timeInBar = time_in_bar(time);
    const musical_time_t previousTime = app->previousTime;
    if (timeInBar == 0) {
        // 小節終わりのノート(ノートオフ想定)を発音する
        if (previousTime > 0) {
            for (const AppDrumPattern::DrumPatternItem &item : pattern)
            {
                if (previousTime < item.time && item.time <= 1920) process_item(item);
                else if (item.time > 1920) break;
            }
        }
        // 小節頭のノートを発音する
        for (const AppDrumPattern::DrumPatternItem &item : pattern)
        {
            if (item.time <= 0) process_item(item);
            else if (item.time > 0) break;
        }
    }
    else
    {
        // previousTimeより後でcurrentTimeと同じかそれより前のノートを発音する
        for (const AppDrumPattern::DrumPatternItem &item : pattern)
        {
            if (previousTime < item.time && item.time <= timeInBar) process_item(item);
            else if (item.time > timeInBar) break;
        }
    }

    app->previousTime = timeInBar;
}
