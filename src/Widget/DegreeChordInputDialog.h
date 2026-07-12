#pragma once

#include <lvgl.h>
#include <functional>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include "Chord.h"
#include "CustomKeySettings.h"
#include "Keypad.h"

// カスタムキー割り当ての入力ダイアログ。
// キーパッドでコードを入力するとプレビューが更新され（閉じない）、
// 7/M7スイッチでセブンスキー押下時に付与する7度の種類を選び、
// OKで確定・キャンセル/背景タップで破棄する。
class DegreeChordInputDialog {
public:
    using CompleteCallback = std::function<void(CustomKeyAssignment)>;

    DegreeChordInputDialog();
    ~DegreeChordInputDialog();

    void create(const char* title, CustomKeyAssignment currentValue, CompleteCallback onComplete);
    void del();
    void updateIfNeeded();
    bool getShown() const { return isShown; }

private:
    class InputListener : public CapsuleChordKeypad::KeyEventListener {
    public:
        explicit InputListener(DegreeChordInputDialog* dialog) : dialog(dialog) {}
        bool onKeyPressed(uint8_t keyCode) override;
        bool onKeyReleased(uint8_t keyCode) override;
        void deactivate() { active = false; }

    private:
        DegreeChordInputDialog* dialog;
        volatile bool active = true;
        // 自分が消費したpress中の左キー（button番号のビットマスク）。
        // 対応するreleaseだけを消費し、ダイアログ表示前から押されていた
        // キーのreleaseは下のKeyMapへ流す（stopChordを届かせるため）。
        uint16_t consumedLeftKeys = 0;
    };

    static void onBackgroundClicked(lv_event_t* e);
    static void onOkClicked(lv_event_t* e);
    static void onCancelClicked(lv_event_t* e);
    static void onSeventhTypeChanged(lv_event_t* e);

    bool isShown = false;
    bool removeQueued = false;
    volatile bool closeRequested = false;
    volatile bool completed = false;      // OKボタンで確定された
    volatile bool needsUiUpdate = false;  // I2Cスレッドからの入力でUI更新が必要

    lv_obj_t* overlay = nullptr;
    lv_obj_t* frame = nullptr;
    lv_obj_t* titleLabel = nullptr;
    lv_obj_t* previewLabel = nullptr;
    lv_obj_t* seventhMatrix = nullptr;
    lv_obj_t* helpLabel = nullptr;
    lv_obj_t* okButton = nullptr;
    lv_obj_t* cancelButton = nullptr;

    CompleteCallback onComplete;

    // 編集中の値。pendingMutexで保護（I2Cスレッドの入力listenerが書き、メインループが読む）
    DegreeChord pendingChord;
    CustomKeyAssignment::SeventhType pendingSeventhType = CustomKeyAssignment::SeventhType::Seventh;
    portMUX_TYPE pendingMutex = portMUX_INITIALIZER_UNLOCKED;

    std::shared_ptr<InputListener> inputListener;

    void handleChordInput(const DegreeChord& chord);  // I2Cスレッドから呼ばれる
    void applySelectedSeventhType(CustomKeyAssignment::SeventhType type);  // メインループから
    void refreshUi();  // メインループから: プレビューと7/M7選択状態を反映
    void queueRemoveListener();
};
