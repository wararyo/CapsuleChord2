#include "lv_appbutton.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_appbutton_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_appbutton_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_appbutton_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_appbutton_event(const lv_obj_class_t *class_p, lv_event_t *e);
static void draw_main(lv_event_t *e);
static void knock_anim_cb(void *obj, int32_t value);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_appbutton_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_appbutton_constructor,
    .destructor_cb = lv_appbutton_destructor,
    .event_cb = lv_appbutton_event,
    .width_def = 112,
    .height_def = 104,
    .instance_size = sizeof(lv_appbutton_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_appbutton_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_appbutton_set_app(lv_obj_t *obj, AppBase *app)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_appbutton_t *appbutton = (lv_appbutton_t *)obj;
    appbutton->app = app;

    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

AppBase *lv_appbutton_get_app(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_appbutton_t *appbutton = (lv_appbutton_t *)obj;

    return appbutton->app;
}

/*=====================
 * Animation functions
 *====================*/

void lv_appbutton_knock(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    
    lv_appbutton_t *appbutton = (lv_appbutton_t *)obj;
    
    // Start the knock animation
    lv_anim_del(appbutton, knock_anim_cb);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, appbutton);
    lv_anim_set_values(&a, 4, 2);  // Animate border width from 4px to 2px
    lv_anim_set_time(&a, 300);     // Animation duration: 300ms
    lv_anim_set_exec_cb(&a, knock_anim_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

// Animation callback for knock effect
static void knock_anim_cb(void *obj, int32_t value)
{
    lv_appbutton_t *appbutton = (lv_appbutton_t *)obj;
    appbutton->current_width = value;
    lv_obj_invalidate(&appbutton->obj);
}

static void lv_appbutton_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_appbutton_t *appbutton = (lv_appbutton_t *)obj;
    appbutton->app = NULL;
    appbutton->current_width = 2;  // Default border width

    // Style for the container
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_appbutton_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
    // No additional cleanup needed
    lv_anim_del(obj, knock_anim_cb);
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_appbutton_event(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RES_OK)
        return;

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_DRAW_MAIN)
    {
        draw_main(e);
    }
}

static void draw_main(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_appbutton_t *appbutton = (lv_appbutton_t *)obj;
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);

    // Get object coordinates
    lv_area_t obj_coords;
    lv_obj_get_coords(obj, &obj_coords);
    
    // Calculate positions
    lv_coord_t w = lv_area_get_width(&obj_coords);
    lv_coord_t h = lv_area_get_height(&obj_coords);
    
    // Circle center coordinates
    lv_coord_t circle_radius = 28;
    lv_point_t circle_center;
    circle_center.x = obj_coords.x1 + w / 2;
    circle_center.y = obj_coords.y1 + 8 + circle_radius; // 8px padding from top + radius
    
    // Draw the icon circle (border)
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.width = appbutton->current_width;  // Use the animated width
    arc_dsc.color = lv_color_white();
    arc_dsc.opa = appbutton->app && appbutton->app->getActive() ? 
                  LV_OPA_COVER : LV_OPA_50;
    lv_draw_arc(draw_ctx, &arc_dsc, &circle_center, circle_radius, 360, 0);
    
    // Draw app icon if available
    if (appbutton->app && appbutton->app->getIcon()) {
        lv_draw_img_dsc_t img_dsc;
        lv_draw_img_dsc_init(&img_dsc);
        img_dsc.recolor = lv_color_white();
        img_dsc.recolor_opa = LV_OPA_COVER;
        img_dsc.opa = LV_OPA_COVER;
        
        lv_img_dsc_t *icon = appbutton->app->getIcon();
        
        // Calculate icon position to center it within the circle
        lv_coord_t icon_w = icon->header.w;
        lv_coord_t icon_h = icon->header.h;
        
        lv_area_t img_area;
        img_area.x1 = circle_center.x - icon_w / 2;
        img_area.y1 = circle_center.y - icon_h / 2;
        img_area.x2 = img_area.x1 + icon_w - 1;
        img_area.y2 = img_area.y1 + icon_h - 1;
        
        lv_draw_img(draw_ctx, &img_dsc, &img_area, icon);
    }
    
    // Draw the label at the bottom
    if (appbutton->app) {
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = lv_color_white();
        label_dsc.font = &genshin_16;
        label_dsc.align = LV_TEXT_ALIGN_CENTER;
        label_dsc.opa = LV_OPA_COVER;
        
        const char* app_name = appbutton->app->getAppName();
        
        // Calculate text dimensions
        lv_point_t txt_size;
        lv_txt_get_size(&txt_size, app_name, label_dsc.font, 0, 0, LV_COORD_MAX, 0);
        
        // Create label area
        lv_area_t label_area;
        label_area.x1 = obj_coords.x1;
        label_area.x2 = obj_coords.x2;
        label_area.y1 = obj_coords.y2 - 24; // Position 24px from bottom
        label_area.y2 = label_area.y1 + txt_size.y;
        
        // Draw the text
        lv_draw_label(draw_ctx, &label_dsc, &label_area, app_name, NULL);
    }
}