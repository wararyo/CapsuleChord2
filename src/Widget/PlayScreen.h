#pragma once

#include <lvgl.h>
#include "Chord.h"
#include "Widget/TempoDialog.h"
#include "Tempo.h"

// メイン画面のUI要素を管理するクラス
class PlayScreen
{
public:
    PlayScreen();
    ~PlayScreen();
    
    void create();
    void del();
    
    bool isShown() const { return isCreated; }
    
    // UI更新メソッド
    void updateBattery();
    void updateScale();
    void updateScale(const char* scaleText);
    void updateTempo();
    void updateTick(bool isBar);
    void setChord(const Chord& chord);
    
    TempoDialog& getTempoDialog() { return tempoDialog; }

private:
    // UI要素
    lv_obj_t *tickframe;
    lv_obj_t *chordlabel;
    lv_obj_t *battery;
    lv_obj_t *scale_label;
    lv_obj_t *tempo_label;
    lv_obj_t *btn_label_left;   // Label for left button (Key-)
    lv_obj_t *btn_label_center; // Label for center button (Apps)
    lv_obj_t *btn_label_right;  // Label for right button (Key+)
    
    TempoDialog tempoDialog;
    
    bool isCreated = false;
};
