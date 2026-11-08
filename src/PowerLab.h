#pragma once

#include <atomic>
#include "BuildConfig.h"

// 電池持ち改善のための測定補助（docs/battery-life-improvement-plan.md フェーズ0a）
// - 充電禁止: USBテスターで VBUS 電流を測るとき、充電電流が混ざらないようにする
// - 電力ログ: 1分ごとに電池電圧・残量・オーディオ処理負荷を UART へ出す
// CAPSULECHORD_ENABLE_POWER_LAB が 0 のビルドでは何もしない。
class PowerLab {
public:
#if CAPSULECHORD_ENABLE_POWER_LAB
    // 充電禁止を切り替える。反映は I2C スレッドで行う。保存しないので再起動で解除される
    void setChargeDisabled(bool disabled) { chargeDisabled.store(disabled); }
    bool isChargeDisabled() const { return chargeDisabled.load(); }
    // I2C スレッドから毎ループ呼ぶ
    void updateOnI2CThread(unsigned long nowMs);
#else
    void setChargeDisabled(bool) {}
    bool isChargeDisabled() const { return false; }
    void updateOnI2CThread(unsigned long) {}
#endif

private:
#if CAPSULECHORD_ENABLE_POWER_LAB
    static constexpr unsigned long kLogIntervalMs = 60UL * 1000UL;

    void printLog(unsigned long nowMs);

    std::atomic<bool> chargeDisabled{false};
    bool appliedChargeDisabled = false;
    bool headerPrinted = false;
    unsigned long lastLogMs = 0;
#endif
};

extern PowerLab PwrLab;
