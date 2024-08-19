#include "lv_battery.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_battery_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_battery_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_battery_event(const lv_obj_class_t *class_p, lv_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_battery_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_battery_constructor,
    .event_cb = lv_battery_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_battery_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_battery_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_battery_set_level(lv_obj_t *obj, uint_fast8_t level)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_battery_t *batt = (lv_battery_t *)obj;
    batt->battery_level = level;

    lv_obj_refresh_self_size(obj);

    lv_obj_invalidate(obj);
}

void lv_battery_set_charging(lv_obj_t *obj, bool charging)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_battery_t *batt = (lv_battery_t *)obj;
    batt->is_charging = charging;

    lv_obj_refresh_self_size(obj);

    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

uint_fast8_t lv_battery_get_level(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_battery_t *batt = (lv_battery_t *)obj;

    return batt->battery_level;
}

bool lv_battery_get_charging(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_battery_t *batt = (lv_battery_t *)obj;

    return batt->is_charging;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_battery_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_battery_t *batt = (lv_battery_t *)obj;

    batt->battery_level = 75;
    batt->is_charging = false;

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_battery_event(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RES_OK)
        return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_REFR_EXT_DRAW_SIZE)
    {
    }
    else if (code == LV_EVENT_GET_SELF_SIZE)
    {
        lv_point_t *p = (lv_point_t *)lv_event_get_param(e);
        p->x = 32;
        p->y = 16;
    }
    else if (code == LV_EVENT_DRAW_MAIN)
    {
        lv_battery_t *batt = (lv_battery_t *)obj;
        lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);

        lv_area_t area;
        lv_obj_get_coords(obj, &area);

        // Draw the battery frame
        lv_area_t frame_area = {area.x1, area.y1, area.x2 - 2, area.y2};
        lv_draw_rect_dsc_t frame_dsc;
        lv_draw_rect_dsc_init(&frame_dsc);
        lv_obj_init_draw_rect_dsc(obj, LV_PART_MAIN, &frame_dsc);
        frame_dsc.radius = 2;
        frame_dsc.border_width = 2;
        frame_dsc.border_opa = LV_OPA_COVER;
        frame_dsc.border_color = lv_color_white();
        lv_draw_rect(draw_ctx, &frame_dsc, &frame_area);

        // Draw the battery level
        lv_coord_t level_width = (32-2-2-2-1-1) * (batt->battery_level / 100.0);
        if(level_width > 24) level_width = 24;
        else if(level_width < 0) level_width = 0;
        lv_area_t level_area = {area.x1 + (2+1), area.y1 + (2+1), area.x1 + level_width + 2, area.y2 - (2+1)};
        lv_draw_rect_dsc_t level_dsc;
        lv_draw_rect_dsc_init(&level_dsc);
        lv_obj_init_draw_rect_dsc(obj, LV_PART_MAIN, &level_dsc);
        level_dsc.bg_opa = LV_OPA_COVER;
        if (batt->is_charging) level_dsc.bg_color = lv_color_hex(0x00FF00);
        else if (batt->battery_level <= 25) level_dsc.bg_color = lv_color_hex(0xFF0000);
        else level_dsc.bg_color = lv_color_white();
        lv_draw_rect(draw_ctx, &level_dsc, &level_area);

        // Draw the polar
        lv_area_t polar_area = {area.x2 - 2, area.y1 + 4, area.x2, area.y2 - 4};
        lv_draw_rect_dsc_t polar_dsc;
        lv_draw_rect_dsc_init(&polar_dsc);
        lv_obj_init_draw_rect_dsc(obj, LV_PART_MAIN, &polar_dsc);
        polar_dsc.bg_opa = LV_OPA_COVER;
        polar_dsc.bg_color = lv_color_white();
        lv_draw_rect(draw_ctx, &polar_dsc, &polar_area);
    }
}
