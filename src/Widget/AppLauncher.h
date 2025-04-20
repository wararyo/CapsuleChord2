#pragma once

#include <lvgl.h>
#include <vector>
#include <set>
#include "App/AppBase.h"
#include "Context.h"

// アプリ一覧画面
class AppLauncher : public Context::KnockListener
{
public:
    AppLauncher();
    ~AppLauncher();
    
    void create();
    void del();
    void update();
    bool getShown()
    {
        return isShown;
    }
    
    // KnockListener implementation
    void onKnock(AppBase* app) override;

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
    
    // Use a set instead of a vector to automatically handle duplicates
    bool needsKnockAnimation = false;
    std::set<AppBase*> appsToKnock;

    // Find the button widget for a given app
    lv_obj_t* findAppButton(AppBase* app);
};
