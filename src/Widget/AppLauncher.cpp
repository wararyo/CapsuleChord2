#include "AppLauncher.h"
#include "LvglWrapper.h"
#include "App/AppManager.h"

void AppLauncher::create()
{
    frame = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frame, LvglWrapper::screenWidth, LvglWrapper::screenHeight - 24);
    lv_obj_set_style_bg_color(frame, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(frame, 0, 0);
    lv_obj_set_style_border_width(frame, 0, 0);
    lv_obj_set_align(frame, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_flex_flow(frame, LV_FLEX_FLOW_COLUMN);

    // App.appsからアプリ一覧を取得し表示する
    for (AppBase *app : App.apps)
    {
        lv_obj_t * obj;
        lv_obj_t * label;

        obj = lv_btn_create(frame);
        lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
        AppIconClickEventData *eventData = new AppIconClickEventData{this, app};
        eventDatas.push_back(eventData);
        lv_obj_add_event_cb(obj, [](lv_event_t *e)
        {
            auto data = (AppIconClickEventData *)lv_event_get_user_data(e);
            data->launcher->del();
            App.launchApp(data->app);
        }, LV_EVENT_CLICKED, (void *)eventData);

        label = lv_label_create(obj);
        lv_label_set_text(label, app->getAppName());
        lv_obj_center(label);
    }
    isShown = true;
}

void AppLauncher::del()
{
    lv_obj_del(frame);
    frame = nullptr;
    for (AppIconClickEventData *eventData : eventDatas)
    {
        delete eventData;
    }
    eventDatas.clear();
    isShown = false;
}

void AppLauncher::update()
{
}
