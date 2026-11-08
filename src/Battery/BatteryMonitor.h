#pragma once

#include <atomic>
#include <stdint.h>
#include "BatteryEstimator.h"

// 電池の電圧と充電状態を I2C スレッドで定期的に読み、表示する残量を推定する。
// メインループは getLevel() / isCharging() を読むだけで、I2C には触らない
class BatteryMonitor {
public:
    // 電圧を読む間隔。表示の推定値は kAverageCount 回分（約1分）の平均になる
    static constexpr uint32_t kReadIntervalMs = 10 * 1000;

    // I2C スレッドのループから毎回呼ぶ
    void updateOnI2CThread(uint32_t nowMs, bool vbusPresent);

    // 表示する残量（%）。まだ読んでいないときは -1
    int getLevel() const { return level.load(std::memory_order_relaxed); }
    // 充電中か
    bool isCharging() const { return charging.load(std::memory_order_relaxed); }
    // 推定に使った電池電流（mA）。電力ログ用
    int getEstimatedLoadMa() const { return loadMa.load(std::memory_order_relaxed); }

private:
    BatteryEstimator estimator;
    bool hasRead = false;
    uint32_t lastReadMs = 0;
    bool lastVbusPresent = false;

    std::atomic<int> level{-1};
    std::atomic<bool> charging{false};
    std::atomic<int> loadMa{0};
};

extern BatteryMonitor Battery;
