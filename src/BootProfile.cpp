#include "BootProfile.h"

#if CAPSULECHORD_ENABLE_BOOT_PROFILE

#include <inttypes.h>
#include <esp_log.h>
#include <esp_timer.h>

static const char* LOG_TAG = "BootProfile";

void BootProfile::begin() {
    setupStartUs = esp_timer_get_time();
    started = true;
}

void BootProfile::mark(const char* label) {
    if (!started || reported) return;
    if (count >= kMaxEntries) {
        dropped++;
        return;
    }
    entries[count++] = {label, esp_timer_get_time()};
}

void BootProfile::report() {
    if (!started || reported) return;
    reported = true;

    // UsbComposite::initConsole() 後は ESP_LOGx が USB CDC に流れて UART に出なくなるため、
    // VFS を通らず ROM の printf で UART に直接書く ESP_EARLY_LOGx を使う。
    // ロックを取らないので、他タスクの UART 出力と行が混ざることがある。
    // ログレベルを WARN に下げても見えるように W で出す
    ESP_EARLY_LOGW(LOG_TAG, "---- boot profile ----");
    ESP_EARLY_LOGW(LOG_TAG, "%-28s %6" PRId64 " ms", "esp_timer init -> setup()", setupStartUs / 1000);

    int64_t prevUs = setupStartUs;
    for (size_t i = 0; i < count; i++) {
        const Entry& e = entries[i];
        ESP_EARLY_LOGW(LOG_TAG, "%-28s %6" PRId64 " ms (+%5" PRId64 " ms)",
                 e.label, (e.us - setupStartUs) / 1000, (e.us - prevUs) / 1000);
        prevUs = e.us;
    }
    if (dropped > 0) {
        ESP_EARLY_LOGW(LOG_TAG, "%u marks dropped (kMaxEntries=%u)", (unsigned)dropped, (unsigned)kMaxEntries);
    }
    ESP_EARLY_LOGW(LOG_TAG, "%-28s %6" PRId64 " ms", "setup() -> last mark", (prevUs - setupStartUs) / 1000);
    ESP_EARLY_LOGW(LOG_TAG, "----------------------------------------");
}

#endif // CAPSULECHORD_ENABLE_BOOT_PROFILE

BootProfile BootProf;
