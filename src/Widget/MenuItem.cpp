#include "MenuItem.h"
#include "MenuScreen.h"

// 定数
static const int MENU_ITEM_HEIGHT = 32;
static const int MENU_ITEM_PADDING = 8;

// 共通のUI作成処理（行レイアウト）
lv_obj_t* MenuItemBase::createBaseRow(lv_obj_t* parent) {
    lvObj = lv_obj_create(parent);
    lv_obj_set_size(lvObj, lv_pct(100), MENU_ITEM_HEIGHT);
    lv_obj_set_style_bg_color(lvObj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lvObj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(lvObj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(lvObj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(lvObj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(lvObj, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_radius(lvObj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(lvObj, MENU_ITEM_PADDING, LV_PART_MAIN);
    lv_obj_set_style_pad_right(lvObj, MENU_ITEM_PADDING, LV_PART_MAIN);
    lv_obj_set_style_pad_top(lvObj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(lvObj, 0, LV_PART_MAIN);
    lv_obj_clear_flag(lvObj, LV_OBJ_FLAG_SCROLLABLE);

    // 左側のラベル
    lv_obj_t* labelObj = lv_label_create(lvObj);
    lv_label_set_text(labelObj, label);
    lv_obj_set_style_text_color(labelObj, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(labelObj, LV_ALIGN_LEFT_MID, 0, 0);

    // 右側の値表示ラベル
    valueLabel = lv_label_create(lvObj);
    lv_obj_set_style_text_color(valueLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(valueLabel, LV_ALIGN_RIGHT_MID, 0, 0);

    return lvObj;
}

// === MenuItemToggle ===

lv_obj_t* MenuItemToggle::createLvObj(lv_obj_t* parent) {
    createBaseRow(parent);
    updateDisplay();
    return lvObj;
}

void MenuItemToggle::updateDisplay() {
    if (valueLabel) {
        lv_label_set_text(valueLabel, getValue() ? "有効" : "無効");
    }
}

void MenuItemToggle::onClick(MenuScreen* menuScreen) {
    (void)menuScreen;
    setValue(!getValue());
    updateDisplay();
}

// === MenuItemSelection ===

lv_obj_t* MenuItemSelection::createLvObj(lv_obj_t* parent) {
    createBaseRow(parent);
    updateDisplay();
    return lvObj;
}

void MenuItemSelection::updateDisplay() {
    if (valueLabel && !options.empty()) {
        int currentVal = getValue();
        const char* displayText = "";
        for (const auto& opt : options) {
            if (opt.value == currentVal) {
                displayText = opt.label;
                break;
            }
        }
        lv_label_set_text(valueLabel, displayText);
    }
}

void MenuItemSelection::onClick(MenuScreen* menuScreen) {
    if (menuScreen) {
        menuScreen->showSelectionDialog(this);
    }
}

// === MenuItemNavigation ===

lv_obj_t* MenuItemNavigation::createLvObj(lv_obj_t* parent) {
    createBaseRow(parent);
    updateDisplay();
    return lvObj;
}

void MenuItemNavigation::updateDisplay() {
    if (valueLabel) {
        // ナビゲーション型は右側に矢印を表示（または空）
        lv_label_set_text(valueLabel, "");
    }
}

void MenuItemNavigation::onClick(MenuScreen* menuScreen) {
    (void)menuScreen;
    if (onNavigate) {
        onNavigate();
    }
}
