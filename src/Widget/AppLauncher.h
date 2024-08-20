#pragma once

#include <lvgl.h>
#include <vector>
#include "App/AppBase.h"

// アプリ一覧画面
class AppLauncher
{
public:
    void create();
    void del();
    void update();
    bool getShown()
    {
        return isShown;
    }
private:
    struct AppIconClickEventData
    {
        AppLauncher *launcher;
        AppBase *app;
    };
    bool isShown = false;
    lv_obj_t *frame = nullptr;
    // アプリアイコンクリック時に使用するデータ
    // メモリリークを防ぐためにdeleteする必要がある
    std::vector<AppIconClickEventData *> eventDatas;
};
