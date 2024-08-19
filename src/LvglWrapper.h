#pragma once

#include "lvgl.h"

class LvglWrapper
{
public:
    static const uint16_t screenWidth = 240;
    static const uint16_t screenHeight = 320;

    void begin();

    lv_disp_draw_buf_t draw_buf;
    lv_color_t buf[2][screenWidth * 10];
    lv_disp_drv_t disp_drv;
    lv_disp_t *disp;
    lv_indev_drv_t indev_drv;
    lv_style_t scr_style;
};

extern LvglWrapper Lvgl;
