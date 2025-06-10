#include "LvglWrapper.h"
#include "M5Unified.h"
#include "I2CHandler.h"

static void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    if (M5.Display.getStartCount() == 0)
    {   // Processing if not yet started
        M5.Display.startWrite();
    }
    M5.Display.pushImageDMA( area->x1
                    , area->y1
                    , area->x2 - area->x1 + 1
                    , area->y2 - area->y1 + 1
                    , ( lgfx::swap565_t* )&color_p->full);
    lv_disp_flush_ready( disp );
}
static void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    data->state = LV_INDEV_STATE_REL;

    I2CHandler::TouchData touchData;
    if (I2C.getTouchData(&touchData) && touchData.isTouched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchData.x;
        data->point.y = touchData.y;
        I2C.resetTouchData();
    }
}

void LvglWrapper::begin()
{
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf[0], buf[1], screenWidth * 10);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp = lv_disp_drv_register(&disp_drv);
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    lv_disp_set_default(disp);
    lv_log_register_print_cb([](const char *buf) {
        Serial.print(buf);
    });

    lv_style_init(&scr_style);
    lv_style_set_bg_color(&scr_style, lv_color_black());
    lv_obj_add_style(lv_scr_act(), &scr_style, 0);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
}

LvglWrapper Lvgl;
