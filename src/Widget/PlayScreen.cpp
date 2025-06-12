#include "PlayScreen.h"
#include <M5Unified.h>
#include "Scale.h"
#include "Widget/lv_chordlabel.h"
#include "Widget/lv_battery.h"
#include "Widget/lv_tickframe.h"

PlayScreen::PlayScreen()
{
    tickframe = nullptr;
    chordlabel = nullptr;
    battery = nullptr;
    scale_label = nullptr;
    tempo_label = nullptr;
    btn_label_left = nullptr;
    btn_label_center = nullptr;
    btn_label_right = nullptr;
}

PlayScreen::~PlayScreen()
{
    del();
}

void PlayScreen::create()
{
    if (isCreated) return;
    
    // LVGLウィジェットの初期化
    tickframe = lv_tickframe_create(lv_scr_act());
    lv_obj_set_size(tickframe, 240, 320);
    lv_obj_align(tickframe, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(tickframe, LV_OBJ_FLAG_CLICKABLE);
    
    chordlabel = lv_chordlabel_create(lv_scr_act());
    lv_chordlabel_set_chord(chordlabel, Chord());
    lv_obj_center(chordlabel);
    
    battery = lv_battery_create(lv_scr_act());
    lv_obj_align(battery, LV_ALIGN_TOP_RIGHT, -4, 4);
    
    scale_label = lv_label_create(lv_scr_act());
    lv_obj_align(scale_label, LV_ALIGN_TOP_LEFT, 4, 28);
    
    tempo_label = lv_label_create(lv_scr_act());
    lv_obj_align(tempo_label, LV_ALIGN_TOP_RIGHT, -8, 32);
    lv_obj_add_flag(tempo_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(tempo_label, 16);
    lv_obj_add_event_cb(tempo_label, [](lv_event_t *e) {
        PlayScreen* playScreen = (PlayScreen*)lv_event_get_user_data(e);
        playScreen->tempoDialog.create();
    }, LV_EVENT_CLICKED, (void*)this);

    // ボタン操作説明用ラベルの初期化
    btn_label_left = lv_label_create(lv_scr_act());
    lv_label_set_text(btn_label_left, "Key-");
    lv_obj_align(btn_label_left, LV_ALIGN_BOTTOM_MID, -80, -8);

    btn_label_center = lv_label_create(lv_scr_act());
    lv_label_set_text(btn_label_center, "Apps");
    lv_obj_align(btn_label_center, LV_ALIGN_BOTTOM_MID, 0, -8);

    btn_label_right = lv_label_create(lv_scr_act());
    lv_label_set_text(btn_label_right, "Key+");
    lv_obj_align(btn_label_right, LV_ALIGN_BOTTOM_MID, 80, -8);
    
    isCreated = true;
}

void PlayScreen::del()
{
    if (!isCreated) return;
    
    if (tickframe) {
        lv_obj_del(tickframe);
        tickframe = nullptr;
    }
    if (chordlabel) {
        lv_obj_del(chordlabel);
        chordlabel = nullptr;
    }
    if (battery) {
        lv_obj_del(battery);
        battery = nullptr;
    }
    if (scale_label) {
        lv_obj_del(scale_label);
        scale_label = nullptr;
    }
    if (tempo_label) {
        lv_obj_del(tempo_label);
        tempo_label = nullptr;
    }
    if (btn_label_left) {
        lv_obj_del(btn_label_left);
        btn_label_left = nullptr;
    }
    if (btn_label_center) {
        lv_obj_del(btn_label_center);
        btn_label_center = nullptr;
    }
    if (btn_label_right) {
        lv_obj_del(btn_label_right);
        btn_label_right = nullptr;
    }
    
    isCreated = false;
}

void PlayScreen::updateBattery()
{
    if (!isCreated || !battery) return;
    
    // バッテリー残量を取得
    int32_t level = M5.Power.getBatteryLevel();
    bool isCharging = M5.Power.isCharging();
    lv_battery_set_level(battery, level);
    lv_battery_set_charging(battery, isCharging);
}

void PlayScreen::updateScale()
{
    if (!isCreated || !scale_label) return;
    
    // このメソッドは外部からScale情報を渡してもらう必要があります
    // main.cppでupdate_scale()を呼び出す際に、直接lv_label_set_textを使用します
}

void PlayScreen::updateScale(const char* scaleText)
{
    if (!isCreated || !scale_label) return;
    
    lv_label_set_text(scale_label, scaleText);
}

void PlayScreen::updateTempo()
{
    if (!isCreated || !tempo_label) return;
    
    char text[64] = {'\0'};
    sprintf(text, "%d 4/4", Tempo.getTempo());
    lv_label_set_text(tempo_label, text);
}

void PlayScreen::updateTick(bool isBar)
{
    if (!isCreated || !tickframe) return;
    
    lv_tickframe_tick(tickframe, isBar);
}

void PlayScreen::setChord(const Chord& chord)
{
    if (!isCreated || !chordlabel) return;
    
    lv_chordlabel_set_chord(chordlabel, chord);
}
