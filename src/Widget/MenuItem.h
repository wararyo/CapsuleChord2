#pragma once

#include <lvgl.h>
#include <functional>
#include <vector>
#include <string>

// 前方宣言
class MenuScreen;

// メニュー項目タイプ
enum class MenuItemType {
    Toggle,      // ON/OFF トグル
    Selection,   // 複数選択肢
    Navigation   // サブメニュー/情報画面へ遷移
};

// メニュー項目の基底クラス
class MenuItemBase {
public:
    MenuItemBase(const char* label) : label(label) {}
    virtual ~MenuItemBase() = default;

    virtual MenuItemType getType() const = 0;
    const char* getLabel() const { return label; }

    // 親コンテナにLVGLオブジェクトを作成
    virtual lv_obj_t* createLvObj(lv_obj_t* parent) = 0;

    // 表示を更新（値変更時など）
    virtual void updateDisplay() = 0;

    // クリック時の処理
    virtual void onClick(MenuScreen* menuScreen) = 0;

    // LVGLオブジェクトを取得
    lv_obj_t* getLvObj() const { return lvObj; }

protected:
    const char* label;
    lv_obj_t* lvObj = nullptr;      // この項目のLVGLオブジェクト
    lv_obj_t* valueLabel = nullptr; // 右側の値表示ラベル

    // 共通のUI作成処理（行レイアウト）
    lv_obj_t* createBaseRow(lv_obj_t* parent);
};

// トグル型メニュー項目（ON/OFF）
class MenuItemToggle : public MenuItemBase {
public:
    MenuItemToggle(const char* label,
                   std::function<bool()> getter,
                   std::function<void(bool)> setter)
        : MenuItemBase(label), getValue(getter), setValue(setter) {}

    MenuItemType getType() const override { return MenuItemType::Toggle; }
    lv_obj_t* createLvObj(lv_obj_t* parent) override;
    void updateDisplay() override;
    void onClick(MenuScreen* menuScreen) override;

private:
    std::function<bool()> getValue;
    std::function<void(bool)> setValue;
};

// 選択型メニュー項目（複数選択肢）
class MenuItemSelection : public MenuItemBase {
public:
    struct Option {
        const char* label;
        int value;
    };

    MenuItemSelection(const char* label,
                      const std::vector<Option>& options,
                      std::function<int()> getter,
                      std::function<void(int)> setter)
        : MenuItemBase(label), options(options), getValue(getter), setValue(setter) {}

    MenuItemType getType() const override { return MenuItemType::Selection; }
    lv_obj_t* createLvObj(lv_obj_t* parent) override;
    void updateDisplay() override;
    void onClick(MenuScreen* menuScreen) override;

    const std::vector<Option>& getOptions() const { return options; }
    int getCurrentValue() const { return getValue(); }
    void setCurrentValue(int value) { setValue(value); updateDisplay(); }

private:
    std::vector<Option> options;
    std::function<int()> getValue;
    std::function<void(int)> setValue;
};

// ナビゲーション型メニュー項目（別画面へ遷移）
class MenuItemNavigation : public MenuItemBase {
public:
    MenuItemNavigation(const char* label,
                       std::function<void()> onNavigate)
        : MenuItemBase(label), onNavigate(onNavigate) {}

    MenuItemType getType() const override { return MenuItemType::Navigation; }
    lv_obj_t* createLvObj(lv_obj_t* parent) override;
    void updateDisplay() override;
    void onClick(MenuScreen* menuScreen) override;

private:
    std::function<void()> onNavigate;
};
