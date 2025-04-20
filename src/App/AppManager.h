#pragma once

#include <list>
#include "AppBase.h"
#include "AppMetronome.h"
#include "AppDrumPattern.h"
#include "AppSequencer.h"
#include "AppBass.h"
#include "AppXyPad.h"
#include "AppDucking.h"
#include "AppSoundTest.h"
#include "../Context.h"

class AppManager
{
public:
    AppManager() {
        for (AppBase *app : apps)
        {
            app->onCreate();
        }
    }
    
    // Initialize context for all apps
    void initContext(Context *context) {
        for (AppBase *app : apps)
        {
            app->setContext(context);
        }
    }
    
    const std::list<AppBase *> apps = {
        new AppMetronome(),
        new AppDrumPattern(),
        new AppSequencer(),
        new AppBass(),
        new AppXyPad(),
        new AppDucking(),
        new AppSoundTest(),
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
