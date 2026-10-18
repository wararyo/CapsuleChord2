#include "LoopStats.h"

#if CAPSULECHORD_ENABLE_LOOP_STATS

#include <algorithm>
#include <esp_log.h>
#include <esp_timer.h>

static const char* LOG_TAG = "LoopStats";

static inline unsigned long esp_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void LoopStats::beginLvgl() {
    lvglStartMs = esp_millis();
}

void LoopStats::endLvgl() {
    lvglMs = esp_millis() - lvglStartMs;
}

void LoopStats::recordLoop(unsigned long nowMs, unsigned long loopMs) {
    loopCount++;
    maxLoopMs = std::max(maxLoopMs, loopMs);
    maxLvglMs = std::max(maxLvglMs, lvglMs);
    if (loopMs >= kSlowLoopMs) {
        slowLoopCount++;
        if (slowStreakStartMs == 0) slowStreakStartMs = nowMs - loopMs;
        maxSlowStreakMs = std::max(maxSlowStreakMs, nowMs - slowStreakStartMs);
    } else {
        slowStreakStartMs = 0;
    }

    if (nowMs - windowStartMs < kReportIntervalMs) return;
    // 重いループがあった区間だけ出力する
    if (slowLoopCount > 0) {
        ESP_LOGW(LOG_TAG, "loop: n=%u slow=%u max=%lums lvgl_max=%lums slow_streak_max=%lums",
                 (unsigned)loopCount, (unsigned)slowLoopCount, maxLoopMs, maxLvglMs, maxSlowStreakMs);
    }
    windowStartMs = nowMs;
    loopCount = 0;
    slowLoopCount = 0;
    maxLoopMs = 0;
    maxLvglMs = 0;
    maxSlowStreakMs = 0;
}

#endif // CAPSULECHORD_ENABLE_LOOP_STATS

LoopStats MainLoopStats;
