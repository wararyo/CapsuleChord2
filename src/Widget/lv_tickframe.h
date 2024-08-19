#pragma once

#include <lvgl.h>

// 画面の周辺に表示する、テンポに合わせて光る枠
typedef struct
{
    lv_obj_t obj;
    bool is_bar;                 // 小節のTickかどうか
    uint8_t current_opacity;     // 現在の透明度
    lv_area_t invalidatearea[4]; // 再描画領域
} lv_tickframe_t;

extern const lv_obj_class_t lv_tickframe_class;

/**
 * Create a tickframe object
 * @param parent pointer to an object, it will be the parent of the new line
 * @return pointer to the created line
 */
lv_obj_t *lv_tickframe_create(lv_obj_t *parent);

/**
 * Trigger a tick.
 * @param obj           pointer to a line object
 */
void lv_tickframe_tick(lv_obj_t *obj, bool bar);
