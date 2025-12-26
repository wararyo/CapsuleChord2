#pragma once

#include <lvgl.h>
#include "Chord.h"
#include "ChordPipeline.h"
#include "Widget/TempoDialog.h"
#include "Tempo.h"
#include "Output/MidiOutput.h"

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

    // メインループから呼び出す更新処理
    void update();
    
    // 出力先を順に切り替え
    void cycleOutput();

    TempoDialog& getTempoDialog() { return tempoDialog; }

private:
    // コード表示用フィルター
    class ChordFilter : public ChordPipeline::ChordFilter {
    public:
        PlayScreen* screen = nullptr;
        bool modifiesChord() override { return false; }
        void onChordOn(Chord chord) override;
        void onChordOff() override {}
    };

    // テンポ/ティック表示用コールバック
    class TempoCallbackHandler : public TempoController::TempoCallbacks {
    public:
        PlayScreen* screen = nullptr;
        void onPlayingStateChanged(bool isPlaying) override {}
        void onTempoChanged(TempoController::tempo_t tempo) override;
        void onTick(TempoController::tick_timing_t timing, musical_time_t time) override;
        TempoController::tick_timing_t getTimingMask() override {
            return TempoController::TICK_TIMING_BAR | TempoController::TICK_TIMING_FULL;
        }
    };

    ChordFilter chordFilter;
    TempoCallbackHandler tempoCallback;

    // TickFrameの更新フラグ（コールバックからUI更新を遅延実行するため）
    bool needsTickUpdate = false;
    TempoController::tick_timing_t lastTickTiming = 0;

    // 出力先切り替えの遅延実行用フラグ
    bool pendingOutputChange = false;
    OutputType pendingOutputType = OutputType::Internal;

    // UI要素
    lv_obj_t *tickframe;
    lv_obj_t *chordlabel;
    lv_obj_t *battery;
    lv_obj_t *scale_label;
    lv_obj_t *tempo_label;
    lv_obj_t *output_label;     // Label for output device selection
    lv_obj_t *btn_label_left;   // Label for left button (Key-)
    lv_obj_t *btn_label_center; // Label for center button (Apps)
    lv_obj_t *btn_label_right;  // Label for right button (Key+)

    TempoDialog tempoDialog;

    bool isCreated = false;
};
