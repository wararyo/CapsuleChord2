#pragma once

#include <lvgl.h>

typedef struct {
    lv_obj_t obj;
    uint_fast8_t battery_level;
    bool is_charging;
} lv_battery_t;

extern const lv_obj_class_t lv_battery_class;

/**
 * Create a battery object
 * @param parent pointer to an object, it will be the parent of the new line
 * @return pointer to the created line
 */
lv_obj_t * lv_battery_create(lv_obj_t * parent);

/**
 * Set a battery level.
 * @param obj           pointer to a line object
 * @param level         a battery level
 */
void lv_battery_set_level(lv_obj_t * obj, uint_fast8_t level);
uint_fast8_t lv_battery_get_level(lv_obj_t * obj);

/**
 * Set a charging state.
 * @param obj           pointer to a line object
 * @param is_charging   a charging state
 */
void lv_battery_set_charging(lv_obj_t * obj, bool charging);
bool lv_battery_get_charging(lv_obj_t * obj);
