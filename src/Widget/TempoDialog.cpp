#include "TempoDialog.h"
#include "Tempo.h"

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

void TempoDialog::update()
{
    char tempoStr[16] = {0};
    snprintf(tempoStr, sizeof(tempoStr), "%d", Tempo.getTempo());
    lv_label_set_text(tempo_label, tempoStr);
}

void TempoDialog::create()
{
    // UI
    bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, 240, 320);
    lv_obj_set_style_radius(bg, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(bg, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bg, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bg, LV_OPA_50, LV_PART_MAIN);
    lv_obj_align(bg, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(bg, [](lv_event_t *e)
                        { ((TempoDialog *)lv_event_get_user_data(e))->del(); }, LV_EVENT_CLICKED, (void *)this);

    frame = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frame, 208, 180);
    lv_obj_center(frame);

    tempo_label = lv_label_create(frame);
    lv_label_set_text(tempo_label, "0");
    lv_obj_set_width(tempo_label, 64);
    lv_obj_set_style_text_align(tempo_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(tempo_label, LV_ALIGN_TOP_MID, 0, 16);

    tempo_button_plus = lv_btn_create(frame);
    lv_obj_align(tempo_button_plus, LV_ALIGN_TOP_MID, 64, 0);
    lv_obj_set_size(tempo_button_plus, 48, 48);
    lv_obj_add_event_cb(tempo_button_plus, tempo_button_plus_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_t *label = lv_label_create(tempo_button_plus);
    lv_obj_center(label);
    lv_label_set_text(label, "+");

    tempo_button_minus = lv_btn_create(frame);
    lv_obj_align(tempo_button_minus, LV_ALIGN_TOP_MID, -64, 0);
    lv_obj_set_size(tempo_button_minus, 48, 48);
    lv_obj_add_event_cb(tempo_button_minus, tempo_button_minus_event_cb, LV_EVENT_ALL, NULL);
    label = lv_label_create(tempo_button_minus);
    lv_obj_center(label);
    lv_label_set_text(label, "-");

    button_play = lv_btn_create(frame);
    lv_obj_align(button_play, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(button_play, button_play_event_cb, LV_EVENT_CLICKED, NULL);
    label = lv_label_create(button_play);
    lv_label_set_text(label, "Play");

    update();

    // テンポが変更された際に表示を更新する
    tempoControllerCallbacks.dialog = this;
    Tempo.addListener(&tempoControllerCallbacks);
}

void TempoDialog::del()
{
    lv_obj_del(bg);
    lv_obj_del(frame);
    bg = nullptr;
    frame = nullptr;
    tempo_label = nullptr;
    tempo_button_plus = nullptr;
    tempo_button_minus = nullptr;
    button_play = nullptr;
    Tempo.removeListener(&tempoControllerCallbacks);
}
