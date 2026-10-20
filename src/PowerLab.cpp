#include "PowerLab.h"

#if CAPSULECHORD_ENABLE_POWER_LAB

#include <M5Unified.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <esp_pm.h>
#include "Output/MidiOutput.h"

static const char* LOG_TAG = "PowerLab";

// light sleep で実際に寝ていた時間の合計。PM の light sleep 終了コールバックで加算する
static portMUX_TYPE lightSleepMux = portMUX_INITIALIZER_UNLOCKED;
static DRAM_ATTR uint64_t lightSleepUs = 0;

// light sleep からの復帰時に呼ばれる。クリティカルセクション内なので、加算するだけにする
static esp_err_t IRAM_ATTR onLightSleepExit(int64_t sleptUs, void*) {
    portENTER_CRITICAL_SAFE(&lightSleepMux);
    lightSleepUs += sleptUs;
    portEXIT_CRITICAL_SAFE(&lightSleepMux);
    return ESP_OK;
}

static uint64_t takeLightSleepUs() {
    portENTER_CRITICAL(&lightSleepMux);
    uint64_t us = lightSleepUs;
    lightSleepUs = 0;
    portEXIT_CRITICAL(&lightSleepMux);
    return us;
}

void PowerLab::updateOnI2CThread(unsigned long nowMs) {
    // AXP2101 の充電許可ビットは M5.begin() で初期化されず、ESP32 の再起動をまたいで残る。
    // 初回は必ず書き込み、前回の起動で充電禁止にしたままにならないようにする
    bool disabled = chargeDisabled.load();
    if (!headerPrinted || disabled != appliedChargeDisabled) {
        M5.Power.setBatteryCharge(!disabled);
        appliedChargeDisabled = disabled;
        ESP_LOGI(LOG_TAG, "Battery charge %s", disabled ? "disabled" : "enabled");
    }

    if (!headerPrinted || nowMs - lastLogMs >= kLogIntervalMs) {
        printLog(nowMs);
        lastLogMs = nowMs;
    }
}

void PowerLab::printLog(unsigned long nowMs) {
    // 通常起動では ESP_LOG の出力先が USB CDC に切り替わり、電池駆動時はログが消える。
    // ROM の printf はコンソール UART へ直接出るので、DebugProbe 側で常に拾える。
    if (!headerPrinted) {
        esp_rom_printf("PWRLOG,uptime_s,vbat_mV,level_pct,vbus_mV,charging,charge_disabled,"
                       "audio_avg_us,audio_max_us,audio_load_pct,audio_active_pct,light_sleep_pct\n");
        headerPrinted = true;
        esp_pm_sleep_cbs_register_config_t cbs = {};
        cbs.exit_cb = onLightSleepExit;
        esp_pm_light_sleep_register_cbs(&cbs);
    }

    int vbat = M5.Power.getBatteryVoltage();
    int level = M5.Power.getBatteryLevel();
    int vbus = M5.Power.getVBUSVoltage();
    int charging = static_cast<int>(M5.Power.isCharging());

    // 出力先が内蔵音源以外のときや、無音時休止中はオーディオループが動いていないので 0 になる。
    // load は前回ログからの経過時間に対するオーディオ処理時間の割合、
    // active はオーディオループが動いていた（休止していなかった）時間の割合
    auto audio = Output.Internal.takeAudioLoadStats();
    constexpr uint64_t periodUs = SAMPLE_BUFFER_SIZE * 1000000ULL / SAMPLE_RATE;
    uint64_t elapsedUs = (uint64_t)(nowMs - lastLogMs) * 1000ULL;
    uint32_t loadPermille = 0;
    uint32_t activePermille = 0;
    if (elapsedUs > 0) {
        loadPermille = (uint32_t)((uint64_t)audio.sumUs * 1000ULL / elapsedUs);
        activePermille = (uint32_t)((uint64_t)audio.count * periodUs * 1000ULL / elapsedUs);
        if (activePermille > 1000) activePermille = 1000;
    }
    // light sleep で寝ていた時間の割合
    uint64_t sleptUs = takeLightSleepUs();
    uint32_t sleepPermille = 0;
    if (elapsedUs > 0) {
        sleepPermille = (uint32_t)(sleptUs * 1000ULL / elapsedUs);
        if (sleepPermille > 1000) sleepPermille = 1000;
    }

    esp_rom_printf("PWRLOG,%u,%d,%d,%d,%d,%d,%u,%u,%u.%u,%u.%u,%u.%u\n",
                   (unsigned)(nowMs / 1000UL), vbat, level, vbus, charging,
                   appliedChargeDisabled ? 1 : 0,
                   (unsigned)audio.avgUs, (unsigned)audio.maxUs,
                   (unsigned)(loadPermille / 10), (unsigned)(loadPermille % 10),
                   (unsigned)(activePermille / 10), (unsigned)(activePermille % 10),
                   (unsigned)(sleepPermille / 10), (unsigned)(sleepPermille % 10));
}

#endif // CAPSULECHORD_ENABLE_POWER_LAB

PowerLab PwrLab;
