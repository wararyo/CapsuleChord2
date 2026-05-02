#pragma once

#include <stdint.h>
#include "SettingsStore.h"

class DisplayController {
public:
    void begin();
    void end();

    // 一時的に画面を暗くする（Settings には書き込まない）。
    // 無操作タイムアウト時の dim 表示用。
    void dim();
    // Settings の brightness 値を再適用して dim() を解除する。
    void restore();

private:
    static void applyBrightnessLevel(uint8_t level);

    SettingDescriptor<uint8_t>::SubscriptionToken brightnessToken = 0;
};

extern DisplayController Display;
