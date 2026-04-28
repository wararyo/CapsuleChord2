#pragma once

#include <lvgl.h>
#include <vector>
#include <memory>
#include "MenuItem.h"
#include "SelectionDialog.h"

// メニューカテゴリ
struct MenuCategory {
    const char* name;                                    // カテゴリ名（例: "システム"）
    const lv_img_dsc_t* icon;                           // タブアイコン（将来用）
    std::vector<std::unique_ptr<MenuItemBase>> items;   // メニュー項目
};

class MenuScreen {
public:
    MenuScreen();
    ~MenuScreen();

    void create();
    void del();
    void update();
    bool getShown() const { return isShown; }

    // 選択ダイアログを表示
    void showSelectionDialog(MenuItemSelection* item);

    // 選択ダイアログを閉じる
    void closeSelectionDialog();

private:
    bool isShown = false;

    // UI要素
    lv_obj_t* frame = nullptr;           // メインコンテナ
    lv_obj_t* headerBar = nullptr;       // 上部バー（戻るボタン＋タブ）
    lv_obj_t* backButton = nullptr;      // 戻るボタン
    lv_obj_t* tabContainer = nullptr;    // タブコンテナ
    lv_obj_t* categoryTitle = nullptr;   // カテゴリ名表示
    lv_obj_t* itemContainer = nullptr;   // メニュー項目リスト（スクロール可能）

    // タブボタン
    std::vector<lv_obj_t*> tabButtons;

    // カテゴリとメニュー項目
    std::vector<MenuCategory> categories;
    int currentCategoryIndex = 0;

    // 選択ダイアログ
    SelectionDialog selectionDialog;

    // カテゴリを初期化
    void initializeCategories();

    // 各カテゴリのメニュー項目を構築
    void buildControlsCategory();
    void buildPerformanceCategory();
    void buildVoicingCategory();
    void buildOutputCategory();
    void buildDisplayCategory();
    void buildSystemCategory();

    // カテゴリを切り替え
    void switchToCategory(int index);

    // 現在のカテゴリの項目リストを再構築
    void rebuildItemList();

    // タブのハイライト更新
    void updateTabHighlight();

    // イベントハンドラ
    static void onBackButtonClicked(lv_event_t* e);
    static void onTabClicked(lv_event_t* e);
    static void onItemClicked(lv_event_t* e);
};
