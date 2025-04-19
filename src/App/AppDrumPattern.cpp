#include "AppDrumPattern.h"

const uint8_t patternInUi[] = {
0b00001001,
0b00000000,
0b00001000,
0b00000000,
0b00001010,
0b00000000,
0b00001000,
0b00000000,
0b00001001,
0b00000000,
0b00001001,
0b00000000,
0b00001010,
0b00000000,
0b00001000,
0b00000000,
};

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
    lv_obj_align(switchButton, LV_ALIGN_TOP_RIGHT, 0, 0);
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

    // パターン表示
    patternContainer = lv_obj_create(container);
    lv_obj_center(patternContainer);
    lv_obj_set_size(patternContainer, 222, 144);
    lv_obj_clear_flag(patternContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(patternContainer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(patternContainer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(patternContainer, 0, 0);
    lv_obj_set_style_radius(patternContainer, 0, 0);
    lv_obj_set_style_pad_all(patternContainer, 0, 0);
    // カーソル
    cursorRect = lv_obj_create(patternContainer);
    lv_obj_set_size(cursorRect, 12, 144);
    lv_obj_clear_flag(cursorRect, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(cursorRect, lv_color_hex(0xFFFF66), 0);
    lv_obj_set_style_bg_opa(cursorRect, 127, 0);
    lv_obj_set_style_border_width(cursorRect, 0, 0);
    lv_obj_set_style_radius(cursorRect, 0, 0);
    // 16x8の正方形
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 8; y++)
        {
            lv_obj_t *rect = lv_obj_create(patternContainer);
            lv_obj_set_size(rect, 12, 12);
            lv_obj_align(rect, LV_ALIGN_BOTTOM_LEFT, x * 14, -16 - (y * 14));
            lv_obj_clear_flag(rect, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(rect, 0, 0);
            if (patternInUi[x] & (0b1 << y))
            {
                lv_obj_set_style_border_width(rect, 0, 0);
                lv_obj_set_style_bg_color(rect, lv_color_hex(0xFFFFFF), 0);
                lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
            }
            else
            {
                lv_obj_set_style_border_width(rect, 1, 0);
                lv_obj_set_style_border_color(rect, lv_color_hex(0x999999), 0);
                lv_obj_set_style_border_opa(rect, 127, 0);
                lv_obj_set_style_bg_opa(rect, 0, 0);
            }
        }
    }

    update();
    isShowingGui = true;
}

void AppDrumPattern::onHideGui()
{
    isShowingGui = false;
    lv_obj_del(titleLabel);
    lv_obj_del(switchButton);
    lv_obj_del(patternContainer);
    cursorRect = nullptr;
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

void AppDrumPattern::TempoCallbacks::processItem(const AppDrumPattern::DrumPatternItem &item)
{
    if ((item.status & 0xF0) == 0x90)
    {
        app->context->pipeline->sendNote(true, item.data1, item.data2, item.status & 0x0F);
        shouldKnock = true;
    }
    else if ((item.status & 0xF0) == 0x80)
    {
        app->context->pipeline->sendNote(false, item.data1, item.data2, item.status & 0x0F);
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
                if (previousTime < item.time && item.time <= 1920) processItem(item);
                else if (item.time > 1920) break;
            }
        }
        // 小節頭のノートを発音する
        for (const AppDrumPattern::DrumPatternItem &item : pattern)
        {
            if (item.time <= 0) processItem(item);
            else if (item.time > 0) break;
        }
    }
    else
    {
        // previousTimeより後でcurrentTimeと同じかそれより前のノートを発音する
        for (const AppDrumPattern::DrumPatternItem &item : pattern)
        {
            if (previousTime < item.time && item.time <= timeInBar) processItem(item);
            else if (item.time > timeInBar) break;
        }
    }

    // UIのカーソルを更新
    if (timing & TempoController::TICK_TIMING_QUARTER && app->isShowingGui && app->cursorRect != nullptr)
    {
        lv_obj_align(app->cursorRect, LV_ALIGN_BOTTOM_LEFT, (timeInBar / 120) * 14, 0);
    }

    // 必要ならノックを行う
    if (shouldKnock)
    {
        app->context->knock(app);
        shouldKnock = false;
    }

    app->previousTime = timeInBar;
}
