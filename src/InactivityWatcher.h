#pragma once

#include <atomic>
#include <stdint.h>

class InactivityWatcher {
public:
    enum class Stage : uint8_t { Active, Dimmed, ShouldShutdown };

    static constexpr unsigned long kDimMs      = 5UL * 60UL * 1000UL;
    static constexpr unsigned long kShutdownMs = 10UL * 60UL * 1000UL;

    void begin();
    void notify();
    Stage getStage() const;

private:
    std::atomic<unsigned long> lastActivityMs{0};
};

extern InactivityWatcher Inactivity;
