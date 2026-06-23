#include "DegreeChordInputDialog.h"
#include "KeyMap/ChordKeyInput.h"

static const int SCREEN_WIDTH = 240;
static const int SCREEN_HEIGHT = 320;
static const lv_color_t COLOR_BG = lv_color_hex(0x000000);
static const lv_color_t COLOR_FRAME = lv_color_hex(0x202020);
static const lv_color_t COLOR_TEXT = lv_color_hex(0xFFFFFF);
static const lv_color_t COLOR_ACCENT = lv_color_hex(0x00B8D4);

DegreeChordInputDialog::DegreeChordInputDialog() {}

DegreeChordInputDialog::~DegreeChordInputDialog() {
    del();
}

void DegreeChordInputDialog::create(const char* title, DegreeChord currentValue, CompleteCallback onComplete) {
    if (isShown) del();

    this->onComplete = onComplete;
    closeRequested = false;
    completed = false;
    removeQueued = false;
    pendingChord = currentValue;

    overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_align(overlay, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(overlay, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    frame = lv_obj_create(overlay);
    lv_obj_set_size(frame, 216, 136);
    lv_obj_center(frame);
    lv_obj_set_style_bg_color(frame, COLOR_FRAME, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(frame, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(frame, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(frame, 8, LV_PART_MAIN);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    titleLabel = lv_label_create(frame);
    lv_label_set_text(titleLabel, title ? title : "カスタムキー");
    lv_obj_set_style_text_color(titleLabel, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 12);

    currentLabel = lv_label_create(frame);
    std::string currentText = "現在: " + currentValue.toString();
    lv_label_set_text(currentLabel, currentText.c_str());
    lv_obj_set_style_text_color(currentLabel, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_align(currentLabel, LV_ALIGN_TOP_MID, 0, 48);

    helpLabel = lv_label_create(frame);
    lv_label_set_text(helpLabel, "キーパッドでコードを入力");
    lv_obj_set_style_text_color(helpLabel, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_align(helpLabel, LV_ALIGN_BOTTOM_MID, 0, -16);

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
    currentLabel = nullptr;
    helpLabel = nullptr;

    inputListener.reset();
    isShown = false;
    closeRequested = false;
    completed = false;
    onComplete = nullptr;
}

void DegreeChordInputDialog::requestComplete(const DegreeChord& chord) {
    portENTER_CRITICAL(&pendingMutex);
    pendingChord = chord;
    completed = true;
    closeRequested = true;
    portEXIT_CRITICAL(&pendingMutex);
}

void DegreeChordInputDialog::updateIfNeeded() {
    if (!isShown || !closeRequested) return;

    DegreeChord chord;
    bool shouldComplete = false;
    portENTER_CRITICAL(&pendingMutex);
    chord = pendingChord;
    shouldComplete = completed;
    completed = false;
    closeRequested = false;
    portEXIT_CRITICAL(&pendingMutex);

    auto callback = onComplete;
    if (shouldComplete && callback) {
        callback(chord);
    }

    del();
}

bool DegreeChordInputDialog::InputListener::onKeyPressed(uint8_t keyCode) {
    if (!active || !dialog) return false;

    DegreeChordInputResult result = ChordKeyInput::buildDegreeChordFromKantanKey(keyCode);
    if (!result.valid) return false;

    active = false;
    dialog->requestComplete(result.degreeChord);
    return true;
}

bool DegreeChordInputDialog::InputListener::onKeyReleased(uint8_t keyCode) {
    (void)keyCode;
    return active;
}
