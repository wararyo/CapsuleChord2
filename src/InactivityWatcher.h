#pragma once

#include <atomic>

class InactivityWatcher {
public:
    static constexpr unsigned long kTimeoutMs = 10UL * 60UL * 1000UL;

    void begin();
    void notify();
    bool isIdle() const;

private:
    std::atomic<unsigned long> lastActivityMs{0};
};

extern InactivityWatcher Inactivity;
