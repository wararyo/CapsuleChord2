#pragma once

#include <stdint.h>
#include "SettingsStore.h"

class DisplayController {
public:
    void begin();
    void end();

private:
    static void applyBrightnessLevel(uint8_t level);

    SettingDescriptor<uint8_t>::SubscriptionToken brightnessToken = 0;
};

extern DisplayController Display;
