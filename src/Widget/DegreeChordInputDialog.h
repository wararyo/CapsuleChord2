#pragma once

#include <lvgl.h>
#include <functional>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include "Chord.h"
#include "Keypad.h"

class DegreeChordInputDialog {
public:
    using CompleteCallback = std::function<void(DegreeChord)>;

    DegreeChordInputDialog();
    ~DegreeChordInputDialog();

    void create(const char* title, DegreeChord currentValue, CompleteCallback onComplete);
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

    bool isShown = false;
    bool removeQueued = false;
    volatile bool closeRequested = false;
    volatile bool completed = false;

    lv_obj_t* overlay = nullptr;
    lv_obj_t* frame = nullptr;
    lv_obj_t* titleLabel = nullptr;
    lv_obj_t* currentLabel = nullptr;
    lv_obj_t* helpLabel = nullptr;

    CompleteCallback onComplete;
    DegreeChord pendingChord;
    portMUX_TYPE pendingMutex = portMUX_INITIALIZER_UNLOCKED;
    std::shared_ptr<InputListener> inputListener;

    void requestComplete(const DegreeChord& chord);
    void queueRemoveListener();
};
