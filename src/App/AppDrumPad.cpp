#include "AppDrumPad.h"
#include "AppManager.h"
#include "ChordPipeline.h"
#include "Keypad.h"

void AppDrumPad::onCreate()
{
    isActive = false;
    // Create shared_ptr to self for keypad registration
    selfPtr = std::shared_ptr<CapsuleChordKeypad::KeyEventListener>(
        this, [](CapsuleChordKeypad::KeyEventListener*){} // Custom deleter that doesn't delete
    );
}

void AppDrumPad::onActivate()
{
    // Do nothing
}

void AppDrumPad::onDeactivate()
{
    // Do nothing
}

void AppDrumPad::onShowGui(lv_obj_t *container)
{
    isActive = true;
    isShowingGui = true;
    
    // Register as keypad event listener
    Keypad.addKeyEventListener(selfPtr);
    
    // Create app title
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 4);
    
    // Create container for drum pads
    drumPadContainer = lv_obj_create(container);
    lv_obj_set_size(drumPadContainer, 240, 250);
    lv_obj_align(drumPadContainer, LV_ALIGN_CENTER, 0, 20);
    lv_obj_clear_flag(drumPadContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(drumPadContainer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(drumPadContainer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(drumPadContainer, 0, 0);
    lv_obj_set_style_pad_all(drumPadContainer, 0, 0);
    
    // Create labels for drum pads (left side)
    for (int i = 0; i < 9; i++) {
        lv_obj_t *padRect = lv_obj_create(drumPadContainer);
        lv_obj_set_size(padRect, 64, 40);
        lv_obj_clear_flag(padRect, LV_OBJ_FLAG_SCROLLABLE);
        
        // Position in a 3x3 grid (left side)
        int row = i / 3;
        int col = i % 3;
        lv_obj_align(padRect, LV_ALIGN_TOP_LEFT, 12 + col * 76, 10 + row * 50);
        
        // Style
        lv_obj_set_style_bg_color(padRect, lv_color_hex(0x333333), 0);
        lv_obj_set_style_bg_opa(padRect, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(padRect, 2, 0);
        lv_obj_set_style_border_color(padRect, lv_color_hex(0x666666), 0);
        lv_obj_set_style_radius(padRect, 4, 0);
        
        // Create label with pad name
        lv_obj_t *label = lv_label_create(padRect);
        lv_label_set_text(label, leftPads[i].name);
        lv_obj_center(label);
        
        // Store reference to label for highlighting
        padLabels[i] = padRect;
    }
    
    // Instructions label
    lv_obj_t *instructionsLabel = lv_label_create(container);
    lv_label_set_text(instructionsLabel, "キーを押してドラムを演奏");
    lv_obj_align(instructionsLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void AppDrumPad::onHideGui()
{
    isActive = false;
    isShowingGui = false;
    
    // Unregister as keypad event listener
    Keypad.removeKeyEventListener(selfPtr);

    lv_obj_del(titleLabel);
    lv_obj_del(drumPadContainer);
}

void AppDrumPad::onDestroy()
{
    // Make sure we're unregistered from keypad
    if (isActive) {
        Keypad.removeKeyEventListener(selfPtr);
    }
}

void AppDrumPad::onUpdateGui()
{
    // Nothing to update periodically
}

bool AppDrumPad::onKeyPressed(uint8_t keyCode)
{
    // Only handle events if we're active
    if (!isActive) return false;
    
    // Play the corresponding drum sound
    playDrumSound(keyCode);
    
    // Highlight the pad in the UI
    highlightPad(keyCode, true);
    
    App.knock(this);
    
    // Return true to consume the event
    return true;
}

bool AppDrumPad::onKeyReleased(uint8_t keyCode)
{
    // Only handle events if we're active
    if (!isActive) return false;
    
    // Remove highlight from the pad
    highlightPad(keyCode, false);
    
    // Return true to consume the event
    return true;
}

void AppDrumPad::playDrumSound(uint8_t keyCode)
{
    uint8_t noteNo = getNoteForKey(keyCode);
    if (noteNo > 0) {
        // Play drum sound with velocity 100 on channel 9 (standard MIDI drum channel)
        Pipeline.sendNote(true, noteNo, 127, 9);
    }
}

uint8_t AppDrumPad::getNoteForKey(uint8_t keyCode)
{
    // Extract group and button from keyCode
    uint8_t group = (keyCode >> 4) & 0x07;
    uint8_t button = keyCode & 0x0F;
    
    // Adjust button index (0-based)
    if (button > 0) button--;
    
    // Check if valid button index
    if (button >= 9) return 0;
    
    // Return note based on group
    switch (group) {
        case KEY_GROUP_LEFT:
            return leftPads[button].noteNo;
        case KEY_GROUP_RIGHT:
            return rightPads[button].noteNo;
        default:
            return 0;
    }
}

const char* AppDrumPad::getNameForKey(uint8_t keyCode)
{
    // Extract group and button from keyCode
    uint8_t group = (keyCode >> 4) & 0x07;
    uint8_t button = keyCode & 0x0F;
    
    // Adjust button index (0-based)
    if (button > 0) button--;
    
    // Check if valid button index
    if (button >= 9) return "";
    
    // Return name based on group
    switch (group) {
        case KEY_GROUP_LEFT:
            return leftPads[button].name;
        case KEY_GROUP_RIGHT:
            return rightPads[button].name;
        default:
            return "";
    }
}

void AppDrumPad::highlightPad(uint8_t keyCode, bool highlight)
{
    if (!isShowingGui) return;
    
    // Extract group and button from keyCode
    uint8_t group = (keyCode >> 4) & 0x07;
    uint8_t button = keyCode & 0x0F;
    
    // Adjust button index (0-based)
    if (button > 0) button--;

    if (button < 9) {
        lv_obj_t *padRect = padLabels[button];
        if (padRect) {
            if (highlight) {
                // Highlight the pad
                lv_obj_set_style_bg_color(padRect, lv_color_hex(0x00AA00), 0);
            } else {
                // Return to normal color
                lv_obj_set_style_bg_color(padRect, lv_color_hex(0x333333), 0);
            }
        }
    }
}