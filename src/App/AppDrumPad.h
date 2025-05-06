#pragma once

#include "AppBase.h"
#include "Assets/Icons.h"
#include "Keypad.h"

class AppDrumPad : public AppBase, public CapsuleChordKeypad::KeyEventListener
{
public:
    char *getAppName() { return "ドラムパッド"; }
    lv_img_dsc_t *getIcon() override { return (lv_img_dsc_t *)&app_drum; } // Reusing drum app icon
    bool runsInBackground() { return false; }

    bool getActive() { return isActive; }
    void onCreate() override;
    void onActivate() override;
    void onDeactivate() override;
    void onShowGui(lv_obj_t *container) override;
    void onHideGui() override;
    void onDestroy() override;
    void onUpdateGui() override;

    // Key event handlers
    bool onKeyPressed(uint8_t keyCode) override;
    bool onKeyReleased(uint8_t keyCode) override;

private:
    bool isActive = false;
    bool isShowingGui = false;
    std::shared_ptr<CapsuleChordKeypad::KeyEventListener> selfPtr;
    
    // UI elements
    lv_obj_t *titleLabel;
    lv_obj_t *drumPadContainer;
    lv_obj_t *padLabels[9]; // Labels for drum pads
    
    // Drum sound mapping
    struct DrumPad {
        uint8_t noteNo;
        const char* name;
    };
    
    // Mapping of keys to drum sounds
    DrumPad leftPads[9] = {
        {49, "Crash"},
        {49, "Crash"},
        {49, "Crash"},
        {42, "HiHat"},
        {42, "HiHat"},
        {42, "HiHat"},
        {36, "Kick"},
        {38, "Snare"},
        {37, "Rim"},
    };
    
    DrumPad rightPads[9] = {
        {49, "Crash"},
        {49, "Crash"},
        {49, "Crash"},
        {42, "HiHat"},
        {42, "HiHat"},
        {42, "HiHat"},
        {36, "Kick"},
        {38, "Snare"},
        {37, "Rim"},
    };
    
    // Visual feedback helpers
    void highlightPad(uint8_t keyCode, bool highlight);
    void playDrumSound(uint8_t keyCode);
    uint8_t getNoteForKey(uint8_t keyCode);
    const char* getNameForKey(uint8_t keyCode);
};