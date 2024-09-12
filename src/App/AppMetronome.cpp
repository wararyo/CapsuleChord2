#include "AppMetronome.h"

static void tempo_button_plus_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        Tempo.setTempo(Tempo.getTempo() + 1);
        Tempo.stop();
    }
}

static void tempo_button_minus_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        Tempo.setTempo(Tempo.getTempo() - 1);
        Tempo.stop();
    }
}

static void button_play_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        Tempo.getPlaying() ? Tempo.stop() : Tempo.play();
    }
}

void AppMetronome::update()
{
    // 有効/無効スイッチ
    if (isActive) lv_obj_add_state(switchButton, LV_STATE_CHECKED);
    else lv_obj_clear_flag(switchButton, LV_STATE_CHECKED);

    // テンポ表示
    char tempoStr[16] = {0};
    snprintf(tempoStr, sizeof(tempoStr), "%d", Tempo.getTempo());
    lv_label_set_text(tempo_label, tempoStr);
}

void AppMetronome::onCreate()
{
    isActive = false;
}

void AppMetronome::onActivate()
{
    isActive = true;
    Tempo.addListener(&soundTempoCallbacks);
}

void AppMetronome::onDeactivate()
{
    isActive = false;
    Tempo.removeListener(&soundTempoCallbacks);
}

void AppMetronome::onShowGui(lv_obj_t *container)
{
    // アプリタイトル
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 4);

    // 有効/無効スイッチ
    switchButton = lv_switch_create(container);
    lv_obj_set_size(switchButton, 136, 80);
    lv_obj_align(switchButton, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_add_event_cb(switchButton, [](lv_event_t *e) {
        auto *self = (AppMetronome *)lv_event_get_user_data(e);
        auto *sw = lv_event_get_target(e);
        // ここでisActiveを反転させる
        self->isActive = !self->isActive;
        // 以降のisActiveの値は変更後の値
        if (self->isActive)
        {
            Tempo.addListener(&self->soundTempoCallbacks);
            // 再生していないときにメトロノームを有効にした場合は再生も始める
            if (!Tempo.getPlaying()) Tempo.play();
        }
        else
        {
            Tempo.removeListener(&self->soundTempoCallbacks);
        }
        self->update();
    }, LV_EVENT_CLICKED, (void *)this);

    // テンポ設定
    tempoContainer = lv_obj_create(container);
    lv_obj_set_size(tempoContainer, 208, 128);
    lv_obj_align(tempoContainer, LV_ALIGN_BOTTOM_MID, 0, 0);

    tempo_label = lv_label_create(tempoContainer);
    lv_label_set_text(tempo_label, "0");
    lv_obj_set_width(tempo_label, 64);
    lv_obj_set_style_text_align(tempo_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(tempo_label, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *tempo_button_plus = lv_btn_create(tempoContainer);
    lv_obj_align(tempo_button_plus, LV_ALIGN_TOP_MID, 64, 0);
    lv_obj_set_size(tempo_button_plus, 48, 48);
    lv_obj_add_event_cb(tempo_button_plus, tempo_button_plus_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_t *label = lv_label_create(tempo_button_plus);
    lv_obj_center(label);
    lv_label_set_text(label, "+");

    lv_obj_t *tempo_button_minus = lv_btn_create(tempoContainer);
    lv_obj_align(tempo_button_minus, LV_ALIGN_TOP_MID, -64, 0);
    lv_obj_set_size(tempo_button_minus, 48, 48);
    lv_obj_add_event_cb(tempo_button_minus, tempo_button_minus_event_cb, LV_EVENT_ALL, NULL);
    label = lv_label_create(tempo_button_minus);
    lv_obj_center(label);
    lv_label_set_text(label, "-");

    lv_obj_t *button_play = lv_btn_create(tempoContainer);
    lv_obj_align(button_play, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(button_play, button_play_event_cb, LV_EVENT_CLICKED, NULL);
    label = lv_label_create(button_play);
    lv_label_set_text(label, "Play");

    update();

    // テンポが変更された際に表示を更新する
    uiTempoCallbacks.app = this;
    Tempo.addListener(&uiTempoCallbacks);
}

void AppMetronome::onHideGui()
{
    lv_obj_del(switchButton);
    lv_obj_del(tempoContainer);
    Tempo.removeListener(&uiTempoCallbacks);
}

void AppMetronome::onDestroy()
{
}
