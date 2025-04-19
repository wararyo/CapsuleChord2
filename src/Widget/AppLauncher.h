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
        uint32_t pressedAt;
        lv_point_t pressPoint;    // タッチ開始位置
        lv_point_t releasePoint;  // タッチ終了位置
    };
    bool isShown = false;
    bool isScrolling = false; // スクロール中かどうかを追跡するフラグ
    lv_obj_t *frame = nullptr;
    lv_obj_t *header = nullptr;
    lv_obj_t *grid_container = nullptr;
    // アプリアイコンクリック時に使用するデータ
    // メモリリークを防ぐためにdeleteする必要がある
    std::vector<AppIconClickEventData *> eventDatas;
};
