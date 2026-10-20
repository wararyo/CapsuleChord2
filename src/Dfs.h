#pragma once

#include <atomic>

// 動的周波数制御（DFS）と自動 light sleep を管理するクラス。
// ESP_PM_CPU_FREQ_MAX ロックを誰も持っていないときは 80MHz、誰かが持っている間は 240MHz で動く。
// ロックを持つのは、処理中のメインループ（待ち時間だけ手放す）と、無音時休止していないオーディオループ。
// 最低周波数を 80MHz にするのは、APB クロックを 80MHz のまま保つため
// （M5GFX / M5Unified の SPI・I2C は、起動時の APB クロックで分周比を決めている）。
// さらに、どのタスクも動いておらず PM ロックも無いときは light sleep に入る。実際に寝られるのは、
// 減光中にメインループと I2C スレッドの周期が伸び、オーディオループも無音時休止しているときだけ
// （I2S ドライバは i2s_write() の間だけ PM ロックを持つので、I2S 自体は止めなくてよい）。
class DfsController {
public:
    // 起動直後に1回呼ぶ
    void begin();
    // 測定用: false にすると 240MHz 固定に戻す。保存しない
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled.load(); }
    // 測定用: false にすると light sleep に入らなくなる。保存しない
    void setLightSleepEnabled(bool enabled);
    bool isLightSleepEnabled() const { return lightSleepEnabled.load(); }

private:
    static constexpr int kMaxFreqMhz = 240;
    static constexpr int kMinFreqMhz = 80;

    void configure(bool enable, bool lightSleep);

    std::atomic<bool> enabled{false};
    std::atomic<bool> lightSleepEnabled{false};
};

extern DfsController Dfs;
