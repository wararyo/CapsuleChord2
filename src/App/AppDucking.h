#pragma once

#include "AppBase.h"
#include "Assets/Icons.h"

class AppDucking : public AppBase
{
public:
    char *getAppName() override { return "ダッキング"; }
    lv_img_dsc_t *getIcon() override { return nullptr; } // You'll need to add an icon later
    bool runsInBackground() override { return false; }
    bool getActive() override { return isActive; }

    void onCreate() override;
    void onActivate() override;
    void onDeactivate() override;
    void onShowGui(lv_obj_t *container) override;
    void onHideGui() override;
    void onDestroy() override;

private:
    bool isActive = false;
    lv_obj_t *titleLabel = nullptr;
    lv_obj_t *devLabel = nullptr;
};