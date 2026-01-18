#include "MenuScreen.h"
#include "SettingsStore.h"

// 定数
static const int SCREEN_WIDTH = 240;
static const int SCREEN_HEIGHT = 320;
static const int HEADER_HEIGHT = 32;
static const int TITLE_HEIGHT = 32;
static const int ITEM_HEIGHT = 32;
static const int BACK_BUTTON_WIDTH = 32;
static const int TAB_BUTTON_SIZE = 32;
static const lv_color_t COLOR_BG = lv_color_hex(0x000000);
static const lv_color_t COLOR_TEXT = lv_color_hex(0xFFFFFF);
static const lv_color_t COLOR_ACCENT = lv_color_hex(0x800080);
static const lv_color_t COLOR_BORDER = lv_color_hex(0x333333);

// タブクリック時のユーザーデータ
struct TabEventData {
    MenuScreen* screen;
    int index;
};

// メニュー項目クリック時のユーザーデータ
struct ItemEventData {
    MenuScreen* screen;
    MenuItemBase* item;
};

MenuScreen::MenuScreen() {}

MenuScreen::~MenuScreen() {
    if (isShown) {
        del();
    }
}

void MenuScreen::onBackButtonClicked(lv_event_t* e) {
    MenuScreen* screen = static_cast<MenuScreen*>(lv_event_get_user_data(e));
    if (screen) {
        screen->del();
    }
}

void MenuScreen::onTabClicked(lv_event_t* e) {
    TabEventData* data = static_cast<TabEventData*>(lv_event_get_user_data(e));
    if (data && data->screen) {
        data->screen->switchToCategory(data->index);
    }
}

void MenuScreen::onItemClicked(lv_event_t* e) {
    ItemEventData* data = static_cast<ItemEventData*>(lv_event_get_user_data(e));
    if (data && data->screen && data->item) {
        data->item->onClick(data->screen);
    }
}

void MenuScreen::create() {
    if (isShown) return;

    // カテゴリを初期化
    initializeCategories();

    // メインフレーム（フルスクリーン）
    frame = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frame, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_align(frame, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(frame, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(frame, 0, LV_PART_MAIN);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    // ヘッダーバー
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

    // 戻るボタン
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

    // タブコンテナ
    tabContainer = lv_obj_create(headerBar);
    lv_obj_set_size(tabContainer, SCREEN_WIDTH - BACK_BUTTON_WIDTH, HEADER_HEIGHT);
    lv_obj_align(tabContainer, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(tabContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(tabContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tabContainer, 0, LV_PART_MAIN);
    lv_obj_clear_flag(tabContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(tabContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tabContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // タブボタンを作成
    for (size_t i = 0; i < categories.size(); i++) {
        lv_obj_t* tabBtn = lv_btn_create(tabContainer);
        lv_obj_set_size(tabBtn, TAB_BUTTON_SIZE, TAB_BUTTON_SIZE);
        lv_obj_set_style_bg_opa(tabBtn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(tabBtn, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(tabBtn, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_all(tabBtn, 0, LV_PART_MAIN);

        // プレースホルダーとしてカテゴリ番号を表示
        lv_obj_t* tabLabel = lv_label_create(tabBtn);
        char numStr[4];
        snprintf(numStr, sizeof(numStr), "%d", (int)(i + 1));
        lv_label_set_text(tabLabel, numStr);
        lv_obj_set_style_text_color(tabLabel, COLOR_TEXT, LV_PART_MAIN);
        lv_obj_center(tabLabel);

        // イベントデータを作成
        TabEventData* eventData = new TabEventData{this, (int)i};
        lv_obj_set_user_data(tabBtn, eventData);
        lv_obj_add_event_cb(tabBtn, onTabClicked, LV_EVENT_CLICKED, eventData);

        tabButtons.push_back(tabBtn);
    }

    // カテゴリタイトル
    categoryTitle = lv_label_create(frame);
    lv_obj_set_size(categoryTitle, SCREEN_WIDTH, TITLE_HEIGHT);
    lv_obj_align(categoryTitle, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT);
    lv_obj_set_style_text_color(categoryTitle, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_align(categoryTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_top(categoryTitle, 8, LV_PART_MAIN);

    // メニュー項目コンテナ（スクロール可能）
    itemContainer = lv_obj_create(frame);
    lv_obj_set_size(itemContainer, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - TITLE_HEIGHT);
    lv_obj_align(itemContainer, LV_ALIGN_TOP_LEFT, 0, HEADER_HEIGHT + TITLE_HEIGHT);
    lv_obj_set_style_bg_opa(itemContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(itemContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(itemContainer, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(itemContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(itemContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(itemContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(itemContainer, LV_DIR_VER);

    // 初期カテゴリを表示
    switchToCategory(0);

    isShown = true;
}

void MenuScreen::del() {
    if (!isShown) return;
    isShown = false;

    // 選択ダイアログを閉じる
    closeSelectionDialog();

    // タブボタンのイベントデータを解放
    for (lv_obj_t* tabBtn : tabButtons) {
        TabEventData* data = static_cast<TabEventData*>(lv_obj_get_user_data(tabBtn));
        if (data) {
            delete data;
        }
    }
    tabButtons.clear();

    // カテゴリをクリア
    categories.clear();

    // UIを削除
    if (frame) {
        lv_obj_del(frame);
        frame = nullptr;
    }
    headerBar = nullptr;
    backButton = nullptr;
    tabContainer = nullptr;
    categoryTitle = nullptr;
    itemContainer = nullptr;
    currentCategoryIndex = 0;
}

void MenuScreen::update() {
    // 必要に応じて表示を更新
}

void MenuScreen::showSelectionDialog(MenuItemSelection* item) {
    if (!item) return;

    std::vector<SelectionDialog::Option> options;
    for (const auto& opt : item->getOptions()) {
        options.push_back({opt.label, opt.value});
    }

    selectionDialog.create(
        item->getLabel(),
        options,
        item->getCurrentValue(),
        [item](int value) {
            item->setCurrentValue(value);
        }
    );
}

void MenuScreen::closeSelectionDialog() {
    if (selectionDialog.getShown()) {
        selectionDialog.del();
    }
}

void MenuScreen::initializeCategories() {
    categories.clear();

    buildControlsCategory();
    buildPerformanceCategory();
    buildVoicingCategory();
    buildOutputCategory();
    buildDisplayCategory();
    buildSystemCategory();
}

void MenuScreen::buildControlsCategory() {
    MenuCategory category;
    category.name = "操作";
    category.icon = nullptr;

    // カスタムキー1（スタブ）
    category.items.push_back(std::make_unique<MenuItemSelection>(
        "カスタムキー1",
        std::vector<MenuItemSelection::Option>{
            {"なし", 0},
            {"機能1", 1},
            {"機能2", 2}
        },
        []() { return Settings.controls.customKey1.get(); },
        [](int v) { Settings.controls.customKey1.set(static_cast<uint8_t>(v)); }
    ));

    // カスタムキー2（スタブ）
    category.items.push_back(std::make_unique<MenuItemSelection>(
        "カスタムキー2",
        std::vector<MenuItemSelection::Option>{
            {"なし", 0},
            {"機能1", 1},
            {"機能2", 2}
        },
        []() { return Settings.controls.customKey2.get(); },
        [](int v) { Settings.controls.customKey2.set(static_cast<uint8_t>(v)); }
    ));

    categories.push_back(std::move(category));
}

void MenuScreen::buildPerformanceCategory() {
    MenuCategory category;
    category.name = "演奏";
    category.icon = nullptr;

    // 調（キー）
    category.items.push_back(std::make_unique<MenuItemSelection>(
        "調",
        std::vector<MenuItemSelection::Option>{
            {"C", 0}, {"C#/Db", 1}, {"D", 2}, {"D#/Eb", 3},
            {"E", 4}, {"F", 5}, {"F#/Gb", 6}, {"G", 7},
            {"G#/Ab", 8}, {"A", 9}, {"A#/Bb", 10}, {"B", 11}
        },
        []() { return Settings.performance.scale.get().key; },
        [](int v) {
            Scale newScale = Settings.performance.scale.get();
            newScale.key = v;
            Settings.performance.scale.set(newScale);
        }
    ));

    categories.push_back(std::move(category));
}

void MenuScreen::buildVoicingCategory() {
    MenuCategory category;
    category.name = "ボイシング";
    category.icon = nullptr;

    // 目標音高
    category.items.push_back(std::make_unique<MenuItemSelection>(
        "目標音高",
        std::vector<MenuItemSelection::Option>{
            {"C3", 48}, {"C4", 60}, {"C5", 72}
        },
        []() { return Settings.voicing.centerNoteNo.get(); },
        [](int v) { Settings.voicing.centerNoteNo.set(v); }
    ));

    categories.push_back(std::move(category));
}

void MenuScreen::buildOutputCategory() {
    MenuCategory category;
    category.name = "出力";
    category.icon = nullptr;

    // 出力先
    category.items.push_back(std::make_unique<MenuItemSelection>(
        "出力先",
        std::vector<MenuItemSelection::Option>{
            {"内蔵音源", 0},
            {"Bluetooth MIDI", 1},
            {"USB MIDI", 2}
        },
        []() { return Settings.output.outputTarget.get(); },
        [](int v) { Settings.output.outputTarget.set(static_cast<uint8_t>(v)); }
    ));

    // スピーカー音量
    std::vector<MenuItemSelection::Option> volumeOptions;
    for (int i = 0; i <= 31; i++) {
        char* label = new char[4];
        snprintf(label, 4, "%d", i);
        volumeOptions.push_back({label, i});
    }

    category.items.push_back(std::make_unique<MenuItemSelection>(
        "スピーカー音量",
        volumeOptions,
        []() { return Settings.output.speakerVolume.get(); },
        [](int v) { Settings.output.speakerVolume.set(static_cast<uint8_t>(v)); }
    ));

    // ヘッドホン音量
    category.items.push_back(std::make_unique<MenuItemSelection>(
        "ヘッドホン音量",
        volumeOptions,
        []() { return Settings.output.headphoneVolume.get(); },
        [](int v) { Settings.output.headphoneVolume.set(static_cast<uint8_t>(v)); }
    ));

    categories.push_back(std::move(category));
}

void MenuScreen::buildDisplayCategory() {
    MenuCategory category;
    category.name = "表示";
    category.icon = nullptr;

    // 画面の明るさ
    category.items.push_back(std::make_unique<MenuItemSelection>(
        "画面明るさ",
        std::vector<MenuItemSelection::Option>{
            {"明るい", 0},
            {"普通", 1},
            {"暗い", 2}
        },
        []() { return Settings.display.brightness.get(); },
        [](int v) { Settings.display.brightness.set(static_cast<uint8_t>(v)); }
    ));

    categories.push_back(std::move(category));
}

void MenuScreen::buildSystemCategory() {
    MenuCategory category;
    category.name = "システム";
    category.icon = nullptr;

    // 機内モード
    category.items.push_back(std::make_unique<MenuItemToggle>(
        "機内モード",
        []() { return Settings.system.airplaneMode.get(); },
        [](bool v) { Settings.system.airplaneMode.set(v); }
    ));

    // サイレントモード
    category.items.push_back(std::make_unique<MenuItemToggle>(
        "サイレントモード",
        []() { return Settings.system.silentMode.get(); },
        [](bool v) { Settings.system.silentMode.set(v); }
    ));

    // ネットワーク（ナビゲーション、スタブ）
    category.items.push_back(std::make_unique<MenuItemNavigation>(
        "ネットワーク",
        []() { /* TODO: ネットワーク設定画面へ遷移 */ }
    ));

    // 言語
    category.items.push_back(std::make_unique<MenuItemSelection>(
        "言語",
        std::vector<MenuItemSelection::Option>{
            {"日本語", 0},
            {"English", 1}
        },
        []() { return 0; },  // スタブ: 常に日本語
        [](int v) { /* TODO: 言語切り替えを実装 */ }
    ));

    // ライセンス（ナビゲーション）
    category.items.push_back(std::make_unique<MenuItemNavigation>(
        "ライセンス",
        []() { /* TODO: ライセンス情報画面へ遷移 */ }
    ));

    // ファームウェア情報（ナビゲーション）
    category.items.push_back(std::make_unique<MenuItemNavigation>(
        "ファームウェア情報",
        []() { /* TODO: ファームウェア情報画面へ遷移 */ }
    ));

    categories.push_back(std::move(category));
}

void MenuScreen::switchToCategory(int index) {
    if (index < 0 || index >= (int)categories.size()) return;

    currentCategoryIndex = index;

    // タイトルを更新
    lv_label_set_text(categoryTitle, categories[index].name);

    // タブのハイライトを更新
    updateTabHighlight();

    // 項目リストを再構築
    rebuildItemList();
}

void MenuScreen::rebuildItemList() {
    if (!itemContainer) return;

    // 既存のメニュー項目イベントデータを解放
    uint32_t childCount = lv_obj_get_child_cnt(itemContainer);
    for (uint32_t i = 0; i < childCount; i++) {
        lv_obj_t* child = lv_obj_get_child(itemContainer, i);
        ItemEventData* data = static_cast<ItemEventData*>(lv_obj_get_user_data(child));
        if (data) {
            delete data;
        }
    }

    // 既存の子オブジェクトを削除
    lv_obj_clean(itemContainer);

    // 現在のカテゴリのメニュー項目を作成
    if (currentCategoryIndex >= 0 && currentCategoryIndex < (int)categories.size()) {
        MenuCategory& category = categories[currentCategoryIndex];
        for (auto& item : category.items) {
            lv_obj_t* itemObj = item->createLvObj(itemContainer);

            // イベントデータを作成
            ItemEventData* eventData = new ItemEventData{this, item.get()};
            lv_obj_set_user_data(itemObj, eventData);
            lv_obj_add_event_cb(itemObj, onItemClicked, LV_EVENT_CLICKED, eventData);
        }
    }
}

void MenuScreen::updateTabHighlight() {
    for (size_t i = 0; i < tabButtons.size(); i++) {
        if ((int)i == currentCategoryIndex) {
            lv_obj_set_style_bg_color(tabButtons[i], COLOR_ACCENT, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(tabButtons[i], LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_opa(tabButtons[i], LV_OPA_TRANSP, LV_PART_MAIN);
        }
    }
}
