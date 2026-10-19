#pragma once

#include <stddef.h>
#include <stdint.h>
#include "BuildConfig.h"

// 起動時間の計測（docs/boot-time-optimization-plan.md 3.1 参照）
// esp_timer は PSRAM 初期化・メモリテストの後に 0 から数え始めるため、それ以前の時間は含まない。
// mark(label) は直前の mark から label の処理が終わるまでの時間を記録する。
// 記録中はログを出さず、report() でまとめて出力する（UART 出力の待ちで計測値が膨らむのを避けるため）。
// report() 以降の mark() は無視されるので、起動後に再度呼ばれる処理の中に置いても良い。
// CAPSULECHORD_ENABLE_BOOT_PROFILE が 0 のビルドでは何もしない。
class BootProfile {
public:
#if CAPSULECHORD_ENABLE_BOOT_PROFILE
    // setup() 先頭で呼ぶ。以降の mark() はここからの経過時間で記録される
    void begin();
    void mark(const char* label);
    void report();
#else
    void begin() {}
    void mark(const char*) {}
    void report() {}
#endif

private:
#if CAPSULECHORD_ENABLE_BOOT_PROFILE
    struct Entry {
        const char* label;
        int64_t us;  // esp_timer_get_time()
    };
    static constexpr size_t kMaxEntries = 32;

    Entry entries[kMaxEntries];
    size_t count = 0;
    size_t dropped = 0;
    int64_t setupStartUs = 0;  // esp_timer_get_time()
    bool started = false;
    bool reported = false;
#endif
};

extern BootProfile BootProf;
