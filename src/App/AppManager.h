#pragma once

#include <list>
#include <vector>
#include <memory>
#include <algorithm>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "AppBase.h"
#include "BuildConfig.h"
#include "AppMetronome.h"
#include "AppDrumPattern.h"
#include "AppSequencer.h"
#include "AppBass.h"
#include "AppDrumPad.h"
#if CAPSULECHORD_INCLUDE_DEV_APPS
#include "AppGuide.h"
#include "AppXyPad.h"
#include "AppDucking.h"
#include "AppSoundTest.h"
#include "AppBall.h"
#include "AppAutoPlay.h"
#endif

static const char* LOG_TAG_APP_MANAGER = "AppManager";

class AppManager
{
public:
    // KnockListener interface for receiving knock events from apps
    class KnockListener {
    public:
        virtual void onKnock(AppBase* app) = 0;
        virtual ~KnockListener() = default;
    };

    AppManager() {
        // Initialize apps list with unique_ptr
        apps.push_back(std::make_unique<AppMetronome>());
        apps.push_back(std::make_unique<AppDrumPattern>());
        apps.push_back(std::make_unique<AppSequencer>());
        apps.push_back(std::make_unique<AppBass>());
        apps.push_back(std::make_unique<AppDrumPad>());
#if CAPSULECHORD_INCLUDE_DEV_APPS
        apps.push_back(std::make_unique<AppGuide>());
        apps.push_back(std::make_unique<AppXyPad>());
        apps.push_back(std::make_unique<AppDucking>());
        apps.push_back(std::make_unique<AppSoundTest>());
        apps.push_back(std::make_unique<AppBall>());
        apps.push_back(std::make_unique<AppAutoPlay>());
#endif

        // Knock 通知はアプリごとに 1 ビットで表すため、アプリ数は 32 個まで
        if (apps.size() > kMaxKnockApps) {
            ESP_LOGE(LOG_TAG_APP_MANAGER, "FATAL: Too many apps for knock mask");
        }
        configASSERT(apps.size() <= kMaxKnockApps);
        for (const auto& app : apps)
        {
            if (knockAppCount < kMaxKnockApps) knockApps[knockAppCount++] = app.get();
        }

        for (const auto& app : apps)
        {
            app->onCreate();
        }
    }

    // コピー・ムーブ禁止
    AppManager(const AppManager&) = delete;
    AppManager& operator=(const AppManager&) = delete;
    AppManager(AppManager&&) = delete;
    AppManager& operator=(AppManager&&) = delete;

    std::list<std::unique_ptr<AppBase>> apps;

    // GUIを表示してアプリを起動する
    // ポインタはappsから取得する必要があります
    void launchApp(AppBase *app);
    void hideApp();
    AppBase *getCurrentApp()
    {
        return currentApp;
    }

    // Knock のタスク境界:
    // knock() だけが他タスクから呼べる。登録・解除・配送はメインループ専用。
    // 今後リスナーを追加する際も、通知元タスクから onKnock() を直接呼ばないこと。

    // 登録・解除・配送を許可するタスク（メインループ）を記録する。
    // グローバル初期化時はスケジューラ起動前なので、setup() の先頭で呼ぶ。
    void bindKnockOwnerTask() {
        knockOwnerTask = xTaskGetCurrentTaskHandle();
    }

    void addKnockListener(KnockListener* listener) {
        assertKnockOwnerTask();
        if (listener && std::find(knockListeners.begin(), knockListeners.end(), listener) == knockListeners.end()) {
            knockListeners.push_back(listener);
        }
    }

    void removeKnockListener(KnockListener* listener) {
        assertKnockOwnerTask();
        knockListeners.erase(std::remove(knockListeners.begin(), knockListeners.end(), listener),
            knockListeners.end());
    }

    // 演奏タスクを待たせないよう、アプリに対応するビットを立てるだけにする。
    // 同じアプリの連続した通知は、次の配送までに 1 回へまとめられる。
    void knock(AppBase* app) {
        for (uint32_t i = 0; i < knockAppCount; ++i) {
            if (knockApps[i] == app) {
                pendingKnockMask.fetch_or(1u << i, std::memory_order_relaxed);
                return;
            }
        }
    }

    // メインループから呼び、溜まった Knock をリスナーへ配送する
    void processPendingKnocks();

private:
    // メインループ以外から登録・解除・配送を呼んだ場合に停止させる
    void assertKnockOwnerTask() const {
        configASSERT(knockOwnerTask != nullptr && knockOwnerTask == xTaskGetCurrentTaskHandle());
    }

    static constexpr uint32_t kMaxKnockApps = 32;
    AppBase *currentApp = nullptr;
    lv_obj_t *container = nullptr;
    std::vector<KnockListener*> knockListeners;
    TaskHandle_t knockOwnerTask = nullptr;
    // apps は起動時に生成され以後変化しないため、他タスクからも参照できる
    AppBase* knockApps[kMaxKnockApps] = {};
    uint32_t knockAppCount = 0;
    std::atomic<uint32_t> pendingKnockMask{0};
};

extern AppManager App;
