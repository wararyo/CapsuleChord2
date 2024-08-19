#include "AppManager.h"
#include "LvglWrapper.h"

void AppManager::launchApp(AppBase *app)
{
    if (container == nullptr)
    {
        container = lv_obj_create(lv_scr_act());
        lv_obj_set_size(container, LvglWrapper::screenWidth, LvglWrapper::screenHeight);
        lv_obj_set_style_bg_color(container, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
        lv_obj_set_style_line_width(container, 0, 0);
    }
    if (currentApp != nullptr)
    {
        currentApp->onHideGui();
    }
    currentApp = app;
    currentApp->onCreate();
    currentApp->onShowGui(container);
}

void AppManager::hideApp()
{
    if (currentApp != nullptr)
    {
        currentApp->onHideGui();
        currentApp = nullptr;
    }
}

AppManager App;
