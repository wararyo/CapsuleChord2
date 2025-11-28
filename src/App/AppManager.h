#pragma once

#include <list>
#include <memory>
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
#include "../Context.h"

class AppManager
{
public:
    AppManager() {
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

    // Destructor to ensure proper cleanup (unique_ptr handles this automatically)
    ~AppManager() = default;

    // Initialize context for all apps
    void initContext(Context *context) {
        for (const auto& app : apps)
        {
            app->setContext(context);
        }
    }

    std::list<std::unique_ptr<AppBase>> apps;

    // GUIを表示してアプリを起動する
    // ポインタはappsから取得する必要があります
    void launchApp(AppBase *app);
    void hideApp();
    AppBase *getCurrentApp()
    {
        return currentApp;
    }
private:
    AppBase *currentApp = nullptr;
    lv_obj_t *container = nullptr;
};

extern AppManager App;
