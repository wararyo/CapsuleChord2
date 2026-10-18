#pragma once

#include <stdint.h>
#include "BuildConfig.h"

// メインループ負荷の計測（IDLE1 が実行されず Task WDT になる問題の調査用）
// 重いループがあった区間だけ、1 秒ごとにログを出力する。
// CAPSULECHORD_ENABLE_LOOP_STATS が 0 のビルドでは何もしない。
class LoopStats {
public:
#if CAPSULECHORD_ENABLE_LOOP_STATS
    void beginLvgl();
    void endLvgl();
    // loopMs: 今回のループの処理時間（vTaskDelay による待ち時間を含まない）
    void recordLoop(unsigned long nowMs, unsigned long loopMs);
#else
    void beginLvgl() {}
    void endLvgl() {}
    void recordLoop(unsigned long, unsigned long) {}
#endif

private:
#if CAPSULECHORD_ENABLE_LOOP_STATS
    static constexpr unsigned long kReportIntervalMs = 1000;
    static constexpr unsigned long kSlowLoopMs = 5;  // これ以上かかると以前の実装では vTaskDelay しなかった

    unsigned long windowStartMs = 0;
    unsigned long slowStreakStartMs = 0;  // 0: 重いループが連続していない
    unsigned long lvglStartMs = 0;
    unsigned long lvglMs = 0;
    uint32_t loopCount = 0;
    uint32_t slowLoopCount = 0;
    unsigned long maxLoopMs = 0;
    unsigned long maxLvglMs = 0;
    unsigned long maxSlowStreakMs = 0;
#endif
};

extern LoopStats MainLoopStats;
