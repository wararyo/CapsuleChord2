#include "InactivityWatcher.h"
#include "App/AppManager.h"
#include <esp_timer.h>

static inline unsigned long esp_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

namespace {
struct KnockToInactivity : public AppManager::KnockListener {
    void onKnock(AppBase*) override { Inactivity.notify(); }
};
KnockToInactivity knockAdapter;
}

void InactivityWatcher::begin() {
    lastActivityMs.store(esp_millis(), std::memory_order_relaxed);
    App.addKnockListener(&knockAdapter);
}

void InactivityWatcher::notify() {
    lastActivityMs.store(esp_millis(), std::memory_order_relaxed);
}

InactivityWatcher::Stage InactivityWatcher::getStage() const {
    unsigned long last = lastActivityMs.load(std::memory_order_relaxed);
    unsigned long idle = esp_millis() - last;
    if (idle > kShutdownMs) return Stage::ShouldShutdown;
    if (idle > kDimMs)      return Stage::Dimmed;
    return Stage::Active;
}

InactivityWatcher Inactivity;
