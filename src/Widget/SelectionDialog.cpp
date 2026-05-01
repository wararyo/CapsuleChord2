#include "SelectionDialog.h"

// 定数
static const int OPTION_HEIGHT = 40;
static const int DIALOG_PADDING = 16;
static const int TITLE_HEIGHT = 32;

// オプションクリック時のユーザーデータ
struct OptionEventData {
    SelectionDialog* dialog;
    int value;
};

void SelectionDialog::onBackgroundClicked(lv_event_t* e) {
    SelectionDialog* dialog = static_cast<SelectionDialog*>(lv_event_get_user_data(e));
    if (dialog) {
        dialog->del();
    }
}

void SelectionDialog::onOptionClicked(lv_event_t* e) {
    OptionEventData* data = static_cast<OptionEventData*>(lv_event_get_user_data(e));
    if (data && data->dialog) {
        int value = data->value;
        auto callback = data->dialog->selectionCallback;
        data->dialog->del();
        if (callback) {
            callback(value);
        }
    }
}

void SelectionDialog::create(const char* title,
                              const std::vector<Option>& options,
                              int currentValue,
                              std::function<void(int)> onSelect) {
    if (isShown) return;

    selectionCallback = onSelect;
    selectedValue = currentValue;

    // ダイアログの高さを計算
    int optionCount = options.size();
    int dialogHeight = TITLE_HEIGHT + (optionCount * OPTION_HEIGHT) + DIALOG_PADDING * 2;
    if (dialogHeight > 280) dialogHeight = 280; // 最大高さ制限

    // 半透明背景
    bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, 240, 320);
    lv_obj_set_style_radius(bg, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(bg, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bg, LV_OPA_50, LV_PART_MAIN);
    lv_obj_align(bg, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(bg, onBackgroundClicked, LV_EVENT_CLICKED, this);

    // ダイアログフレーム
    frame = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frame, 200, dialogHeight);
    lv_obj_center(frame);
    lv_obj_set_style_bg_color(frame, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_border_width(frame, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_radius(frame, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(frame, DIALOG_PADDING, LV_PART_MAIN);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    // タイトル
    titleLabel = lv_label_create(frame);
    lv_label_set_text(titleLabel, title);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 0);

    // オプションコンテナ（スクロール可能）
    optionContainer = lv_obj_create(frame);
    lv_obj_set_size(optionContainer, lv_pct(100), dialogHeight - TITLE_HEIGHT - DIALOG_PADDING * 2);
    lv_obj_align(optionContainer, LV_ALIGN_TOP_MID, 0, TITLE_HEIGHT);
    lv_obj_set_style_bg_opa(optionContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(optionContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(optionContainer, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(optionContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(optionContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // オプションボタンを作成
    for (const auto& opt : options) {
        lv_obj_t* optBtn = lv_btn_create(optionContainer);
        lv_obj_set_size(optBtn, lv_pct(100), OPTION_HEIGHT);

        // 現在選択中の値をハイライト
        if (opt.value == currentValue) {
            lv_obj_set_style_bg_color(optBtn, lv_color_hex(0x800080), LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(optBtn, lv_color_hex(0x333333), LV_PART_MAIN);
        }
        lv_obj_set_style_radius(optBtn, 4, LV_PART_MAIN);

        lv_obj_t* optLabel = lv_label_create(optBtn);
        lv_label_set_text(optLabel, opt.label);
        lv_obj_set_style_text_color(optLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_center(optLabel);

        // イベントデータを作成（メモリリークに注意：delで解放）
        OptionEventData* eventData = new OptionEventData{this, opt.value};
        lv_obj_set_user_data(optBtn, eventData);
        lv_obj_add_event_cb(optBtn, onOptionClicked, LV_EVENT_CLICKED, eventData);
    }

    isShown = true;
}

void SelectionDialog::del() {
    if (!isShown) return;
    isShown = false;

    // オプションボタンのイベントデータを解放
    if (optionContainer) {
        uint32_t childCount = lv_obj_get_child_cnt(optionContainer);
        for (uint32_t i = 0; i < childCount; i++) {
            lv_obj_t* child = lv_obj_get_child(optionContainer, i);
            OptionEventData* data = static_cast<OptionEventData*>(lv_obj_get_user_data(child));
            if (data) {
                delete data;
            }
        }
    }

    if (bg) {
        lv_obj_del(bg);
        bg = nullptr;
    }
    if (frame) {
        lv_obj_del(frame);
        frame = nullptr;
    }
    titleLabel = nullptr;
    optionContainer = nullptr;
    selectionCallback = nullptr;
}
