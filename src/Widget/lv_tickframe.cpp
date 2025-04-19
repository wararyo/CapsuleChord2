#include "lv_tickframe.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <M5Unified.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_tickframe_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_tickframe_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_tickframe_event(const lv_obj_class_t *class_p, lv_event_t *e);
static void anim_opa_cb(void *obj, int32_t v);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_tickframe_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_tickframe_constructor,
    .event_cb = lv_tickframe_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_tickframe_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_tickframe_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_tickframe_tick(lv_obj_t *obj, bool bar)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_tickframe_t *frame = (lv_tickframe_t *)obj;

    frame->is_bar = bar;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, frame);
    lv_anim_set_values(&a, LV_OPA_50, 0);
    lv_anim_set_time(&a, 400);
    lv_anim_set_exec_cb(&a, anim_opa_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

/*=====================
 * Getter functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_tickframe_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_tickframe_t *frame = (lv_tickframe_t *)obj;
    frame->current_opacity = 0;
    frame->is_bar = false;

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void anim_opa_cb(void *obj, int32_t v)
{
    lv_tickframe_t *frame = (lv_tickframe_t *)obj;

    frame->current_opacity = v;
    // 再描画領域を更新
    // lv_obj_invalidateを使うと全画面を更新してしまうので、_areaで周辺部のみを再描画する
    lv_obj_invalidate((lv_obj_t *)frame);
    // lv_obj_invalidate_area((lv_obj_t *)frame, &frame->invalidatearea[0]);
    // lv_obj_invalidate_area((lv_obj_t *)frame, &frame->invalidatearea[1]);
    // lv_obj_invalidate_area((lv_obj_t *)frame, &frame->invalidatearea[2]);
    // lv_obj_invalidate_area((lv_obj_t *)frame, &frame->invalidatearea[3]);
}

static void lv_tickframe_event(const lv_obj_class_t *class_p, lv_event_t *e)
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
        lv_tickframe_t *frame = (lv_tickframe_t *)obj;
        lv_area_t area;
        lv_obj_get_coords(obj, &area);
        // 四辺のエリアを取得し、invalidateareaに格納
        frame->invalidatearea[0] = {area.x1, area.y1, area.x1, area.y2};
        frame->invalidatearea[1] = {area.x2, area.y1, area.x2, area.y2};
        frame->invalidatearea[2] = {area.x1, area.y1, area.x2, area.y1};
        frame->invalidatearea[3] = {area.x1, area.y2, area.x2, area.y2};
    }
    else if (code == LV_EVENT_DRAW_MAIN)
    {
        lv_tickframe_t *frame = (lv_tickframe_t *)obj;
        lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);

        lv_area_t area;
        lv_obj_get_coords(obj, &area);

        lv_draw_rect_dsc_t frame_dsc;
        lv_draw_rect_dsc_init(&frame_dsc);
        lv_obj_init_draw_rect_dsc(obj, LV_PART_MAIN, &frame_dsc);
        frame_dsc.radius = 0;
        frame_dsc.border_width = 2;
        frame_dsc.border_opa = frame->current_opacity;
        if (frame->is_bar) frame_dsc.border_color = lv_color_hex(0xFFFF66);
        else frame_dsc.border_color = lv_color_white();
        lv_draw_rect(draw_ctx, &frame_dsc, &area);
    }
}
