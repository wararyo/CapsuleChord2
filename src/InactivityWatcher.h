#pragma once

#include <atomic>
#include <stdint.h>

class InactivityWatcher {
public:
    enum class Stage : uint8_t { Active, Dimmed, ShouldShutdown };

    static constexpr unsigned long kDimMs      = 5UL * 60UL * 1000UL;
    static constexpr unsigned long kShutdownMs = 10UL * 60UL * 1000UL;
    // 減光中のメインループと I2C スレッドの周期。周期を伸ばして、その間 light sleep に入れるようにする
    static constexpr unsigned long kLowPowerLoopIntervalMs = 100;

    void begin();
    void notify();
    Stage getStage() const;
    // 減光以降（省電力のためにループの周期を伸ばす状態）か
    bool isLowPower() const { return getStage() != Stage::Active; }

    // 電池持ち測定用。保存しないので再起動で解除される
    // 無操作による減光・電源OFFを止める
    void setTimeoutDisabled(bool disabled) { timeoutDisabled.store(disabled, std::memory_order_relaxed); }
    bool isTimeoutDisabled() const { return timeoutDisabled.load(std::memory_order_relaxed); }
    // 操作の有無にかかわらず減光状態にする
    void setForceDim(bool force) { forceDim.store(force, std::memory_order_relaxed); }
    bool isForceDim() const { return forceDim.load(std::memory_order_relaxed); }

private:
    std::atomic<unsigned long> lastActivityMs{0};
    std::atomic<bool> timeoutDisabled{false};
    std::atomic<bool> forceDim{false};
};

extern InactivityWatcher Inactivity;
