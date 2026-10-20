#include "Dfs.h"

#include <esp_log.h>
#include <esp_pm.h>

static const char* LOG_TAG = "Dfs";

void DfsController::configure(bool enable) {
    esp_pm_config_t config = {
        .max_freq_mhz = kMaxFreqMhz,
        .min_freq_mhz = enable ? kMinFreqMhz : kMaxFreqMhz,
        .light_sleep_enable = false,
    };
    esp_err_t err = esp_pm_configure(&config);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "esp_pm_configure failed: %s", esp_err_to_name(err));
        return;
    }
    enabled.store(enable);
    ESP_LOGI(LOG_TAG, "DFS %s (%d-%d MHz)", enable ? "enabled" : "disabled",
             config.min_freq_mhz, config.max_freq_mhz);
}

void DfsController::begin() {
    configure(true);
}

void DfsController::setEnabled(bool enable) {
    configure(enable);
}

DfsController Dfs;
