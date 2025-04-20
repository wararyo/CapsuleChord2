#pragma once

#include "AppBase.h"
#include "Assets/Icons.h"

class AppXyPad : public AppBase
{
public:
    char *getAppName() override { return "XYパッド"; }
    lv_img_dsc_t *getIcon() override { return nullptr; } // You'll need to add an icon later
    bool runsInBackground() override { return false; }
    bool getActive() override { return false; }

    void onCreate() override;
    void onActivate() override;
    void onDeactivate() override;
    void onShowGui(lv_obj_t *container) override;
    void onHideGui() override;
    void onDestroy() override;

private:
    lv_obj_t *titleLabel = nullptr;
    lv_obj_t *devLabel = nullptr;
};