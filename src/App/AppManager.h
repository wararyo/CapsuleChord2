#pragma once

#include <list>
#include "AppBase.h"
#include "AppMetronome.h"
#include "AppDrumPattern.h"

class AppManager
{
public:
    AppManager() {
        for (AppBase *app : apps)
        {
            app->onCreate();
        }
    }
    const std::list<AppBase *> apps = {
        new AppMetronome(),
        new AppDrumPattern()
    };
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
