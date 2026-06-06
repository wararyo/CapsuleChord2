#include "InfoScreen.h"

static const int SCREEN_WIDTH = 240;
static const int SCREEN_HEIGHT = 320;
static const int HEADER_HEIGHT = 32;
static const int BACK_BUTTON_WIDTH = 32;
static const lv_color_t COLOR_BG = lv_color_hex(0x000000);
static const lv_color_t COLOR_TEXT = lv_color_hex(0xFFFFFF);
static const lv_color_t COLOR_BORDER = lv_color_hex(0x333333);

InfoScreen::~InfoScreen() {
    if (isShown) {
        del();
    }
}

void InfoScreen::onBackButtonClicked(lv_event_t* e) {
    InfoScreen* screen = static_cast<InfoScreen*>(lv_event_get_user_data(e));
    if (screen) {
        screen->del();
    }
}

void InfoScreen::create(const char* title, const std::string& body) {
    if (isShown) {
        del();
    }

    frame = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frame, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_align(frame, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(frame, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(frame, 0, LV_PART_MAIN);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    headerBar = lv_obj_create(frame);
    lv_obj_set_size(headerBar, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_align(headerBar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(headerBar, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(headerBar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(headerBar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(headerBar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(headerBar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(headerBar, COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_radius(headerBar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(headerBar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(headerBar, LV_OBJ_FLAG_SCROLLABLE);

    backButton = lv_btn_create(headerBar);
    lv_obj_set_size(backButton, BACK_BUTTON_WIDTH, HEADER_HEIGHT);
    lv_obj_align(backButton, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(backButton, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(backButton, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(backButton, onBackButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* backLabel = lv_label_create(backButton);
    lv_label_set_text(backLabel, "<");
    lv_obj_set_style_text_color(backLabel, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_center(backLabel);

    titleLabel = lv_label_create(headerBar);
    lv_label_set_text(titleLabel, title);
    lv_obj_set_style_text_color(titleLabel, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_width(titleLabel, SCREEN_WIDTH - BACK_BUTTON_WIDTH * 2);
    lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(titleLabel, LV_ALIGN_CENTER, 0, 0);

    contentContainer = lv_obj_create(frame);
    lv_obj_set_size(contentContainer, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT);
    lv_obj_align(contentContainer, LV_ALIGN_TOP_LEFT, 0, HEADER_HEIGHT);
    lv_obj_set_style_bg_opa(contentContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(contentContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(contentContainer, 8, LV_PART_MAIN);
    lv_obj_add_flag(contentContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(contentContainer, LV_DIR_VER);

    bodyLabel = lv_label_create(contentContainer);
    lv_label_set_text(bodyLabel, body.c_str());
    lv_obj_set_width(bodyLabel, SCREEN_WIDTH - 16);
    lv_obj_set_style_text_color(bodyLabel, COLOR_TEXT, LV_PART_MAIN);
    lv_label_set_long_mode(bodyLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(bodyLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    isShown = true;
}

void InfoScreen::del() {
    if (!isShown) return;
    isShown = false;

    if (frame) {
        lv_obj_del(frame);
        frame = nullptr;
    }

    headerBar = nullptr;
    backButton = nullptr;
    titleLabel = nullptr;
    contentContainer = nullptr;
    bodyLabel = nullptr;
}
