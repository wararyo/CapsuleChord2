#include "AppLauncher.h"
#include "LvglWrapper.h"
#include "App/AppManager.h"
#include "lv_appbutton.h"

void AppLauncher::create()
{
    frame = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frame, LvglWrapper::screenWidth, LvglWrapper::screenHeight);
    lv_obj_set_style_bg_color(frame, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(frame, 0, 0);
    lv_obj_set_style_border_width(frame, 0, 0);
    lv_obj_set_align(frame, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(frame, 0, 0);
    
    // ヘッダー
    header = lv_obj_create(frame);
    lv_obj_set_size(header, LvglWrapper::screenWidth, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0); // lv_obj_set_alignからlv_obj_alignに変更
    lv_obj_set_style_bg_color(header, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "アプリ");
    lv_obj_set_style_text_font(title, &genshin_16, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);    

    // スクロール可能なグリッドコンテナ
    grid_container = lv_obj_create(frame);
    lv_obj_set_size(grid_container, LvglWrapper::screenWidth, LvglWrapper::screenHeight - 40);
    lv_obj_set_pos(grid_container, 0, 40);
    lv_obj_set_style_bg_color(grid_container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(grid_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(grid_container, 0, 0);
    lv_obj_set_style_radius(grid_container, 0, 0);
    lv_obj_set_style_pad_all(grid_container, 0, 0);
    lv_obj_set_style_pad_row(grid_container, 0, 0);
    lv_obj_set_style_pad_column(grid_container, 0, 0);
    lv_obj_set_flex_flow(grid_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // スクロール検出用のイベントハンドラを追加
    lv_obj_add_event_cb(grid_container, [](lv_event_t *e)
    {
        auto launcher = (AppLauncher *)lv_event_get_user_data(e);
        launcher->isScrolling = true;
    }, LV_EVENT_SCROLL_BEGIN, (void *)this);

    lv_obj_add_event_cb(grid_container, [](lv_event_t *e)
    {
        auto launcher = (AppLauncher *)lv_event_get_user_data(e);
        launcher->isScrolling = false;
    }, LV_EVENT_SCROLL_END, (void *)this);

    // アプリ一覧
    for (AppBase *app : App.apps)
    {
        lv_obj_t *app_button = lv_appbutton_create(grid_container);
        lv_appbutton_set_app(app_button, app);
        
        AppIconClickEventData *eventData = new AppIconClickEventData{this, app, 0, {0, 0}, {0, 0}};
        eventDatas.push_back(eventData);
        
        lv_obj_add_event_cb(app_button, [](lv_event_t *e)
        {
            auto data = (AppIconClickEventData *)lv_event_get_user_data(e);
            // スクロール中はプレスイベントを無視
            if (!data->launcher->isScrolling) {
                data->pressedAt = millis();
                // タッチ位置を記録
                lv_indev_t *indev = lv_indev_get_act();
                if (indev != nullptr) lv_indev_get_point(indev, &data->pressPoint);
            }
        }, LV_EVENT_PRESSED, (void *)eventData);
        
        lv_obj_add_event_cb(app_button, [](lv_event_t *e)
        {
            auto data = (AppIconClickEventData *)lv_event_get_user_data(e);
            // スクロール中はリリースイベントを無視
            if (!data->launcher->isScrolling) {
                // 長押しながらトグル、通常押しでアプリ画面を表示
                if (millis() - data->pressedAt >= 250)
                {
                    if (data->app->getActive()) data->app->onDeactivate();
                    else data->app->onActivate();

                    lv_obj_invalidate(lv_event_get_target(e));
                }
                else
                {
                    // PRESSED時のタッチ位置と現在のタッチ位置が一定以上離れている場合は通常押しとみなさない
                    lv_indev_t *indev = lv_indev_get_act();
                    if (indev != nullptr) {
                        lv_indev_get_point(indev, &data->releasePoint);
                        lv_coord_t dx = data->releasePoint.x - data->pressPoint.x;
                        lv_coord_t dy = data->releasePoint.y - data->pressPoint.y;
                        if (abs(dx) < 10 && abs(dy) < 10){
                            data->launcher->del();
                            App.launchApp(data->app);
                        }
                    }
                }
            }
        }, LV_EVENT_RELEASED, (void *)eventData);
    }
    
    isShown = true;
}

void AppLauncher::del()
{
    lv_obj_del(frame);
    frame = nullptr;
    header = nullptr;
    grid_container = nullptr;
    for (AppIconClickEventData *eventData : eventDatas)
    {
        delete eventData;
    }
    eventDatas.clear();
    isShown = false;
}

void AppLauncher::update()
{
    // アプリの状態の更新が必要であれば、ここで行う
    if (grid_container) {
        // すべてのapp_buttonの状態を更新
        for (uint16_t i = 0; i < lv_obj_get_child_cnt(grid_container); i++) {
            lv_obj_t *app_button = lv_obj_get_child(grid_container, i);
            if (app_button) {
                AppBase *app = lv_appbutton_get_app(app_button);
                if (app) {
                    lv_obj_invalidate(app_button);
                }
            }
        }
    }
}
