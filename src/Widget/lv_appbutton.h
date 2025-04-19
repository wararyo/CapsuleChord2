#pragma once

#include <lvgl.h>
#include "App/AppBase.h"

typedef struct
{
    lv_obj_t obj;
    AppBase *app;
    uint8_t current_width;
} lv_appbutton_t;

extern const lv_obj_class_t lv_appbutton_class;

/**
 * Create an app button object
 * @param parent pointer to an object, it will be the parent of the new app button
 * @return pointer to the created app button
 */
lv_obj_t *lv_appbutton_create(lv_obj_t *parent);

/**
 * Set the app associated with this button
 * @param obj          pointer to an app button object
 * @param app          pointer to the app to associate
 */
void lv_appbutton_set_app(lv_obj_t *obj, AppBase *app);
AppBase *lv_appbutton_get_app(lv_obj_t *obj);

/**
 * Trigger a knock animation on the app button
 * @param obj          pointer to an app button object
 */
void lv_appbutton_knock(lv_obj_t *obj);
