#include "DegreeChordInputDialog.h"
#include "KeyMap/ChordKeyInput.h"
#include "KeyMap/KeyMap.h"
#include "Scale.h"
#include "SettingsStore.h"

static const int SCREEN_WIDTH = 240;
static const int SCREEN_HEIGHT = 320;
// 配色はメニュー内の他のダイアログ(SelectionDialog)に揃える
static const lv_color_t COLOR_BG = lv_color_hex(0x000000);
static const lv_color_t COLOR_FRAME = lv_color_hex(0x222222);
static const lv_color_t COLOR_BORDER = lv_color_hex(0x444444);
static const lv_color_t COLOR_TEXT = lv_color_hex(0xFFFFFF);
static const lv_color_t COLOR_ACCENT = lv_color_hex(0x00B8D4);

// レイアウト(フレーム内側の座標)
static const int FRAME_WIDTH = 216;
static const int FRAME_HEIGHT = 240;
static const int FRAME_PADDING = 12;
static const int TITLE_Y = 6;
static const int PREVIEW_Y = 42;
static const int HELP_Y = 76;
static const int BUTTON_HEIGHT = 40;
static const int SEVENTH_Y = 112;
static const int SEVENTH_WIDTH = 140;
static const int SEVENTH_HEIGHT = BUTTON_HEIGHT;  // キャンセル/OKと揃える
static const int BUTTON_GAP = 10;  // キャンセルとOKの間隔
static const int BUTTON_WIDTH = (FRAME_WIDTH - FRAME_PADDING * 2 - BUTTON_GAP) / 2;

// 7/M7選択のbtnmatrixマップ
static const char* seventhTypeMap[] = {"7", "M7", ""};

DegreeChordInputDialog::DegreeChordInputDialog() {}

DegreeChordInputDialog::~DegreeChordInputDialog() {
    del();
}

void DegreeChordInputDialog::onBackgroundClicked(lv_event_t* e) {
    DegreeChordInputDialog* dialog = static_cast<DegreeChordInputDialog*>(lv_event_get_user_data(e));
    if (dialog && dialog->isShown) {
        dialog->closeRequested = true;  // キャンセル扱い
    }
}

void DegreeChordInputDialog::onOkClicked(lv_event_t* e) {
    DegreeChordInputDialog* dialog = static_cast<DegreeChordInputDialog*>(lv_event_get_user_data(e));
    if (dialog && dialog->isShown) {
        dialog->completed = true;
        dialog->closeRequested = true;
    }
}

void DegreeChordInputDialog::onCancelClicked(lv_event_t* e) {
    DegreeChordInputDialog* dialog = static_cast<DegreeChordInputDialog*>(lv_event_get_user_data(e));
    if (dialog && dialog->isShown) {
        dialog->closeRequested = true;
    }
}

void DegreeChordInputDialog::onSeventhTypeChanged(lv_event_t* e) {
    DegreeChordInputDialog* dialog = static_cast<DegreeChordInputDialog*>(lv_event_get_user_data(e));
    if (!dialog || !dialog->isShown || !dialog->seventhMatrix) return;

    uint16_t id = lv_btnmatrix_get_selected_btn(dialog->seventhMatrix);
    if (id == LV_BTNMATRIX_BTN_NONE) return;
    dialog->applySelectedSeventhType(id == 1
        ? CustomKeyAssignment::SeventhType::MajorSeventh
        : CustomKeyAssignment::SeventhType::Seventh);
}

void DegreeChordInputDialog::create(const char* title, CustomKeyAssignment currentValue, CompleteCallback onComplete) {
    if (isShown) del();

    this->onComplete = onComplete;
    closeRequested = false;
    completed = false;
    needsUiUpdate = false;
    removeQueued = false;
    pendingChord = currentValue.chord;
    pendingSeventhType = currentValue.seventhType;

    overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_align(overlay, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(overlay, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(overlay, onBackgroundClicked, LV_EVENT_CLICKED, this);

    frame = lv_obj_create(overlay);
    lv_obj_set_size(frame, FRAME_WIDTH, FRAME_HEIGHT);
    lv_obj_center(frame);
    lv_obj_set_style_bg_color(frame, COLOR_FRAME, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(frame, COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(frame, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(frame, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(frame, FRAME_PADDING, LV_PART_MAIN);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    // 上から: タイトル / ディグリーネーム / 案内文 / 7・M7スイッチ / (下端)キャンセル・OK
    titleLabel = lv_label_create(frame);
    lv_label_set_text(titleLabel, title ? title : "カスタムキー");
    lv_obj_set_style_text_color(titleLabel, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, TITLE_Y);

    previewLabel = lv_label_create(frame);
    lv_obj_set_style_text_color(previewLabel, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_align(previewLabel, LV_ALIGN_TOP_MID, 0, PREVIEW_Y);

    helpLabel = lv_label_create(frame);
    lv_label_set_text(helpLabel, "キーパッドでコードを入力");
    lv_obj_set_style_text_color(helpLabel, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(helpLabel, LV_ALIGN_TOP_MID, 0, HELP_Y);

    seventhMatrix = lv_btnmatrix_create(frame);
    lv_btnmatrix_set_map(seventhMatrix, seventhTypeMap);
    lv_btnmatrix_set_btn_ctrl_all(seventhMatrix, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(seventhMatrix, true);
    lv_obj_set_size(seventhMatrix, SEVENTH_WIDTH, SEVENTH_HEIGHT);
    lv_obj_align(seventhMatrix, LV_ALIGN_TOP_MID, 0, SEVENTH_Y);
    lv_obj_set_style_bg_opa(seventhMatrix, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(seventhMatrix, 0, LV_PART_MAIN);
    // 排他スイッチであることが伝わるよう、ボタン同士を隙間なく隣接させ、
    // 角丸は外側だけにする（各ボタンは角丸なし、全体を外枠の角丸でクリップ）
    lv_obj_set_style_pad_all(seventhMatrix, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(seventhMatrix, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(seventhMatrix, 4, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(seventhMatrix, true, LV_PART_MAIN);
    lv_obj_set_style_radius(seventhMatrix, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(seventhMatrix, lv_color_hex(0x333333), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(seventhMatrix, COLOR_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(seventhMatrix, COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_add_event_cb(seventhMatrix, onSeventhTypeChanged, LV_EVENT_VALUE_CHANGED, this);

    cancelButton = lv_btn_create(frame);
    lv_obj_set_size(cancelButton, BUTTON_WIDTH, BUTTON_HEIGHT);
    lv_obj_align(cancelButton, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(cancelButton, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_radius(cancelButton, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(cancelButton, onCancelClicked, LV_EVENT_CLICKED, this);
    {
        lv_obj_t* label = lv_label_create(cancelButton);
        lv_label_set_text(label, "キャンセル");
        lv_obj_set_style_text_color(label, COLOR_TEXT, LV_PART_MAIN);
        lv_obj_center(label);
    }

    okButton = lv_btn_create(frame);
    lv_obj_set_size(okButton, BUTTON_WIDTH, BUTTON_HEIGHT);
    lv_obj_align(okButton, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(okButton, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_radius(okButton, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(okButton, onOkClicked, LV_EVENT_CLICKED, this);
    {
        lv_obj_t* label = lv_label_create(okButton);
        lv_label_set_text(label, "OK");
        lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_center(label);
    }

    refreshUi();

    inputListener = std::make_shared<InputListener>(this);
    Keypad.queueAddKeyEventListener(inputListener);

    isShown = true;
}

void DegreeChordInputDialog::queueRemoveListener() {
    if (inputListener && !removeQueued) {
        inputListener->deactivate();
        Keypad.queueRemoveKeyEventListener(inputListener);
        removeQueued = true;
    }
}

void DegreeChordInputDialog::del() {
    if (!isShown && !inputListener) return;

    queueRemoveListener();

    if (overlay) {
        lv_obj_del(overlay);
        overlay = nullptr;
    }
    frame = nullptr;
    titleLabel = nullptr;
    previewLabel = nullptr;
    seventhMatrix = nullptr;
    helpLabel = nullptr;
    okButton = nullptr;
    cancelButton = nullptr;

    inputListener.reset();
    isShown = false;
    closeRequested = false;
    completed = false;
    needsUiUpdate = false;
    onComplete = nullptr;
}

void DegreeChordInputDialog::handleChordInput(const DegreeChord& chord) {
    // 7度の提案: 入力に7度が含まれていればその種類、
    // なければダイアトニック音の場合のみスケールから推論する
    CustomKeyAssignment::SeventhType suggested = CustomKeyAssignment::SeventhType::Seventh;
    bool hasSuggestion = false;
    if (chord.option & Chord::MajorSeventh) {
        suggested = CustomKeyAssignment::SeventhType::MajorSeventh;
        hasSuggestion = true;
    } else if (chord.option & Chord::Seventh) {
        suggested = CustomKeyAssignment::SeventhType::Seventh;
        hasSuggestion = true;
    } else {
        Scale scale = Settings.performance.scale.get();
        int number = scale.getDiatonicNumber(chord.root);
        if (number >= 0) {
            uint16_t options = scale.getDiatonicDegree(number, true).option;
            suggested = (options & Chord::MajorSeventh)
                ? CustomKeyAssignment::SeventhType::MajorSeventh
                : CustomKeyAssignment::SeventhType::Seventh;
            hasSuggestion = true;
        }
    }

    portENTER_CRITICAL(&pendingMutex);
    pendingChord = chord;
    if (hasSuggestion) pendingSeventhType = suggested;
    portEXIT_CRITICAL(&pendingMutex);
    needsUiUpdate = true;
}

void DegreeChordInputDialog::applySelectedSeventhType(CustomKeyAssignment::SeventhType type) {
    portENTER_CRITICAL(&pendingMutex);
    pendingSeventhType = type;
    // 7度が焼き込まれている場合は選択に合わせて書き換える
    if (pendingChord.option & (Chord::Seventh | Chord::MajorSeventh)) {
        applySeventhType(pendingChord, type);
    }
    portEXIT_CRITICAL(&pendingMutex);
    refreshUi();
}

void DegreeChordInputDialog::refreshUi() {
    if (!previewLabel || !seventhMatrix) return;

    DegreeChord chord;
    CustomKeyAssignment::SeventhType type;
    portENTER_CRITICAL(&pendingMutex);
    chord = pendingChord;
    type = pendingSeventhType;
    portEXIT_CRITICAL(&pendingMutex);

    lv_label_set_text(previewLabel, chord.toString().c_str());

    uint16_t checkedId = (type == CustomKeyAssignment::SeventhType::MajorSeventh) ? 1 : 0;
    lv_btnmatrix_set_btn_ctrl(seventhMatrix, checkedId, LV_BTNMATRIX_CTRL_CHECKED);
    lv_btnmatrix_clear_btn_ctrl(seventhMatrix, 1 - checkedId, LV_BTNMATRIX_CTRL_CHECKED);
}

void DegreeChordInputDialog::updateIfNeeded() {
    if (!isShown) return;

    if (needsUiUpdate) {
        needsUiUpdate = false;
        refreshUi();
    }

    if (!closeRequested) return;

    bool shouldComplete = completed;

    DegreeChord chord;
    CustomKeyAssignment::SeventhType type;
    portENTER_CRITICAL(&pendingMutex);
    chord = pendingChord;
    type = pendingSeventhType;
    portEXIT_CRITICAL(&pendingMutex);

    auto callback = onComplete;
    del();
    if (shouldComplete && callback) {
        callback(CustomKeyAssignment(chord, type));
    }
}

bool DegreeChordInputDialog::InputListener::onKeyPressed(uint8_t keyCode) {
    if (!active || !dialog) return false;
    if ((keyCode & 0xF0) != 0x00) return false;  // 左キーパッド以外は関与しない

    // 使用中のキーマップと同じ配置で入力できるよう、現在のテーブルで解釈する
    KeyInputResult result = ChordKeyInput::resolveKey(keyCode, KeyMap::getCurrentKeyMap()->getNumberKeyMap());
    consumedLeftKeys |= (1u << (keyCode & 0x0F));

    if (result.kind == KeyInputResult::Kind::Degree) {
        dialog->handleChordInput(result.degree);
    }
    // ダイアログ表示中はカスタムキー等も含め左キーパッドの押下を下のKeyMapへ流さない
    return true;
}

bool DegreeChordInputDialog::InputListener::onKeyReleased(uint8_t keyCode) {
    if ((keyCode & 0xF0) != 0x00) return false;

    uint16_t bit = 1u << (keyCode & 0x0F);
    if (consumedLeftKeys & bit) {
        consumedLeftKeys &= ~bit;
        return true;  // 自分が消費したpressに対応するreleaseのみ消費
    }
    return false;
}
