#pragma once

#include <list>
#include <vector>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include "AppBase.h"
#include "AppMetronome.h"
#include "AppDrumPattern.h"
#include "AppSequencer.h"
#include "AppBass.h"
#include "AppGuide.h"
#include "AppXyPad.h"
#include "AppDrumPad.h"
#include "AppDucking.h"
#include "AppSoundTest.h"
#include "AppBall.h"
#include "AppAutoPlay.h"

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
        knockMutex = xSemaphoreCreateMutex();
        if (!knockMutex) {
            ESP_LOGE(LOG_TAG_APP_MANAGER, "FATAL: Failed to create knockMutex");
        }

        // Initialize apps list with unique_ptr
        apps.push_back(std::make_unique<AppMetronome>());
        apps.push_back(std::make_unique<AppDrumPattern>());
        apps.push_back(std::make_unique<AppSequencer>());
        apps.push_back(std::make_unique<AppBass>());
        apps.push_back(std::make_unique<AppGuide>());
        apps.push_back(std::make_unique<AppXyPad>());
        apps.push_back(std::make_unique<AppDrumPad>());
        apps.push_back(std::make_unique<AppDucking>());
        apps.push_back(std::make_unique<AppSoundTest>());
        apps.push_back(std::make_unique<AppBall>());
        apps.push_back(std::make_unique<AppAutoPlay>());

        for (const auto& app : apps)
        {
            app->onCreate();
        }
    }

    ~AppManager() {
        if (knockMutex) {
            vSemaphoreDelete(knockMutex);
            knockMutex = nullptr;
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

    // Knock system - アプリ間通信
    void addKnockListener(KnockListener* listener) {
        if (listener && knockMutex) {
            xSemaphoreTake(knockMutex, portMAX_DELAY);
            knockListeners.push_back(listener);
            xSemaphoreGive(knockMutex);
        }
    }

    void removeKnockListener(KnockListener* listener) {
        if (!knockMutex) return;
        xSemaphoreTake(knockMutex, portMAX_DELAY);
        for (auto it = knockListeners.begin(); it != knockListeners.end(); ++it) {
            if (*it == listener) {
                knockListeners.erase(it);
                break;
            }
        }
        xSemaphoreGive(knockMutex);
    }

    void knock(AppBase* app) {
        if (!app || !knockMutex) return;

        xSemaphoreTake(knockMutex, portMAX_DELAY);
        auto listenersCopy = knockListeners;
        xSemaphoreGive(knockMutex);

        for (auto listener : listenersCopy) {
            listener->onKnock(app);
        }
    }

private:
    AppBase *currentApp = nullptr;
    lv_obj_t *container = nullptr;
    std::vector<KnockListener*> knockListeners;
    SemaphoreHandle_t knockMutex = nullptr;
};

extern AppManager App;
