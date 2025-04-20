#include "AppXyPad.h"

void AppXyPad::onCreate()
{
}

void AppXyPad::onActivate()
{
}

void AppXyPad::onDeactivate()
{
}

void AppXyPad::onShowGui(lv_obj_t *container)
{
    // アプリタイトル
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 20);

    // 開発中メッセージ
    devLabel = lv_label_create(container);
    lv_label_set_text(devLabel, "開発中です!");
    lv_obj_align(devLabel, LV_ALIGN_CENTER, 0, 0);
}

void AppXyPad::onHideGui()
{
    lv_obj_del(titleLabel);
    lv_obj_del(devLabel);
    titleLabel = nullptr;
    devLabel = nullptr;
}

void AppXyPad::onDestroy()
{
}