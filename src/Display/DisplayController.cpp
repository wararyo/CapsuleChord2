#include "DisplayController.h"
#include <M5Unified.h>

DisplayController Display;

void DisplayController::begin() {
    applyBrightnessLevel(Settings.display.brightness.get());
    brightnessToken = Settings.display.brightness.subscribe(
        [](const uint8_t&, const uint8_t& newVal) {
            applyBrightnessLevel(newVal);
        });
}

void DisplayController::end() {
    if (brightnessToken != 0) {
        Settings.display.brightness.unsubscribe(brightnessToken);
        brightnessToken = 0;
    }
}

void DisplayController::dim() {
    M5.Lcd.setBrightness(32);
}

void DisplayController::restore() {
    applyBrightnessLevel(Settings.display.brightness.get());
}

void DisplayController::applyBrightnessLevel(uint8_t level) {
    switch (level) {
        case 0: M5.Lcd.setBrightness(255); break;
        case 1: M5.Lcd.setBrightness(127); break;
        case 2: M5.Lcd.setBrightness(32);  break;
        default: M5.Lcd.setBrightness(127); break;
    }
}
