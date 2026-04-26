#include "InactivityWatcher.h"
#include <esp_timer.h>

static inline unsigned long esp_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void InactivityWatcher::begin() {
    lastActivityMs.store(esp_millis(), std::memory_order_relaxed);
}

void InactivityWatcher::notify() {
    lastActivityMs.store(esp_millis(), std::memory_order_relaxed);
}

bool InactivityWatcher::isIdle() const {
    unsigned long last = lastActivityMs.load(std::memory_order_relaxed);
    return (esp_millis() - last) > kTimeoutMs;
}

InactivityWatcher Inactivity;
