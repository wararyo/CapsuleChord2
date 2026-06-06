#pragma once

#include <lvgl.h>
#include <string>

class InfoScreen {
public:
    InfoScreen() = default;
    ~InfoScreen();

    void create(const char* title, const std::string& body);
    void del();
    bool getShown() const { return isShown; }

private:
    bool isShown = false;
    lv_obj_t* frame = nullptr;
    lv_obj_t* headerBar = nullptr;
    lv_obj_t* backButton = nullptr;
    lv_obj_t* titleLabel = nullptr;
    lv_obj_t* contentContainer = nullptr;
    lv_obj_t* bodyLabel = nullptr;

    static void onBackButtonClicked(lv_event_t* e);
};
