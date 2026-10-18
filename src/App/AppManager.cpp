#include "AppManager.h"
#include "LvglWrapper.h"

void AppManager::processPendingKnocks()
{
    assertKnockOwnerTask();
    uint32_t mask = pendingKnockMask.exchange(0, std::memory_order_relaxed);
    if (mask == 0) return;
    const auto listeners = knockListeners;
    for (; mask != 0; mask &= mask - 1) {
        AppBase* app = knockApps[__builtin_ctz(mask)];
        for (KnockListener* listener : listeners) {
            // コールバック内で解除されたリスナーには配送しない
            if (std::find(knockListeners.begin(), knockListeners.end(), listener) != knockListeners.end()) {
                listener->onKnock(app);
            }
        }
    }
}

void AppManager::launchApp(AppBase *app)
{
    if (container == nullptr)
    {
        container = lv_obj_create(lv_scr_act());
        lv_obj_set_size(container, LvglWrapper::screenWidth, LvglWrapper::screenHeight);
        lv_obj_set_align(container, LV_ALIGN_BOTTOM_LEFT);
        lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(container, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_set_style_radius(container, 0, 0);
    }
    if (currentApp != nullptr)
    {
        currentApp->onHideGui();
    }
    currentApp = app;
    currentApp->onShowGui(container);
}

void AppManager::hideApp()
{
    if (currentApp != nullptr)
    {
        currentApp->onHideGui();
        lv_obj_del(container);
        container = nullptr;
        currentApp = nullptr;
    }
}

AppManager App;
