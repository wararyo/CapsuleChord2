#include "BatteryMonitor.h"
#include <M5Unified.h>
#include "InactivityWatcher.h"
#include "SettingsStore.h"

BatteryMonitor Battery;

void BatteryMonitor::updateOnI2CThread(uint32_t nowMs, bool vbusPresent) {
    // USB の抜き差しは、充電中の表示を早く反映するためにすぐ読む
    bool due = !hasRead || nowMs - lastReadMs >= kReadIntervalMs || vbusPresent != lastVbusPresent;
    if (!due) return;
    hasRead = true;
    lastReadMs = nowMs;
    lastVbusPresent = vbusPresent;

    BatteryEstimator::Reading r;
    r.nowMs = nowMs;
    r.mv = M5.Power.getBatteryVoltage();
    r.powered = vbusPresent;
    r.charging = M5.Power.isCharging() == m5::Power_Class::is_charging;
    r.load.dimmed = Inactivity.isLowPower();
    r.load.screenBrightness = Settings.display.brightness.get();

    level.store(estimator.update(r), std::memory_order_relaxed);
    charging.store(r.charging, std::memory_order_relaxed);
    loadMa.store(r.powered ? 0 : BatteryModel::estimateLoadMa(r.load), std::memory_order_relaxed);
}
