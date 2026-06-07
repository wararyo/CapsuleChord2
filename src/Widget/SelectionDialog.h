#pragma once

#include <lvgl.h>
#include <vector>
#include <functional>

class SelectionDialog {
public:
    struct Option {
        const char* label;
        int value;
    };

    void create(const char* title,
                const std::vector<Option>& options,
                int currentValue,
                std::function<void(int)> onSelect);
    void del();
    void requestClose();
    void updateIfNeeded();
    bool getShown() const { return isShown; }

private:
    bool isShown = false;
    bool closeRequested = false;
    bool selectionRequested = false;
    lv_obj_t* bg = nullptr;
    lv_obj_t* frame = nullptr;
    lv_obj_t* titleLabel = nullptr;
    lv_obj_t* optionContainer = nullptr;

    std::function<void(int)> selectionCallback;
    int selectedValue = 0;

    static void onBackgroundClicked(lv_event_t* e);
    static void onOptionClicked(lv_event_t* e);
};
