#ifndef _SETTINGS_STORE_H_
#define _SETTINGS_STORE_H_

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <list>
#include <vector>
#include <functional>
#include <string>
#include <esp_log.h>
#include "Archive.h"
#include "Scale.h"
#include "LittleFSManager.h"

static const char* LOG_TAG_SETTINGS_STORE = "SettingsStore";

// 設定変更リスナーインターフェース
class SettingsListener {
public:
    virtual ~SettingsListener() = default;
    virtual void onSettingChanged(const char* key) = 0;
};

// 前方宣言
class SettingsStore;

// 設定項目テンプレート
template<typename T>
class SettingDescriptor {
private:
    const char* key;
    T value;
    T defaultValue;
    std::list<std::function<void(const T&, const T&)>> listeners;
    portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    bool* categoryDirtyFlag;  // 所属カテゴリのdirtyフラグへの参照
    SettingsStore* store;     // グローバルリスナー通知用

public:
    SettingDescriptor(const char* key, T defaultValue, bool* dirtyFlag, SettingsStore* store)
        : key(key), value(defaultValue), defaultValue(defaultValue),
          categoryDirtyFlag(dirtyFlag), store(store) {}

    const char* getKey() const { return key; }

    const T& get() const { return value; }

    void set(const T& newValue);

    // 変更の購読（戻り値は将来のunsubscribe用）
    size_t subscribe(std::function<void(const T&, const T&)> callback) {
        portENTER_CRITICAL(&mutex);
        listeners.push_back(callback);
        size_t token = listeners.size();
        portEXIT_CRITICAL(&mutex);
        return token;
    }

    // シリアライズ
    void serialize(OutputArchive& archive) const;

    // デシリアライズ
    void deserialize(InputArchive& archive);

    // デフォルト値にリセット
    void reset() {
        set(defaultValue);
    }

    // 内部用: 通知なしで値を設定（デシリアライズ用）
    void setValueWithoutNotify(const T& newValue) {
        value = newValue;
    }
};

// 設定カテゴリの基底クラス
class SettingsCategoryBase {
protected:
    const char* filePath;  // 例: "/settings/controls.json"
    bool isDirty = false;

    // 派生クラスでオーバーライド: 各設定項目のシリアライズ
    virtual void serializeItems(OutputArchive& archive) const = 0;
    virtual void deserializeItems(InputArchive& archive) = 0;

public:
    SettingsCategoryBase(const char* path) : filePath(path) {}
    virtual ~SettingsCategoryBase() = default;

    bool load();   // 共通のファイル読み込みロジック
    bool save();   // 共通のファイル書き込みロジック
    void markDirty() { isDirty = true; }
    bool needsSave() const { return isDirty; }
    const char* getFilePath() const { return filePath; }
};

// 操作設定
class ControlsSettings : public SettingsCategoryBase {
public:
    SettingDescriptor<uint8_t> customKey1;    // カスタムキー1 (スタブ)
    SettingDescriptor<uint8_t> customKey2;    // カスタムキー2 (スタブ)
    SettingDescriptor<uint8_t> leftTrigger;   // Lトリガー (旧Function1, スタブ)
    SettingDescriptor<uint8_t> rightTrigger;  // Rトリガー (旧Function2, スタブ)

    ControlsSettings(SettingsStore* store);

protected:
    void serializeItems(OutputArchive& archive) const override;
    void deserializeItems(InputArchive& archive) override;
};

// 演奏設定
class PerformanceSettings : public SettingsCategoryBase {
public:
    SettingDescriptor<Scale> scale;  // 調

    PerformanceSettings(SettingsStore* store);

protected:
    void serializeItems(OutputArchive& archive) const override;
    void deserializeItems(InputArchive& archive) override;
};

// ボイシング設定
class VoicingSettings : public SettingsCategoryBase {
public:
    SettingDescriptor<int> centerNoteNo;  // 目標音高

    VoicingSettings(SettingsStore* store);

protected:
    void serializeItems(OutputArchive& archive) const override;
    void deserializeItems(InputArchive& archive) override;
};

// 出力設定
class OutputSettings : public SettingsCategoryBase {
public:
    SettingDescriptor<uint8_t> outputTarget;     // 出力先 (0:内蔵音源, 1:BLE, 2:USB)
    SettingDescriptor<uint8_t> speakerVolume;    // スピーカー音量 (0-31)
    SettingDescriptor<uint8_t> headphoneVolume;  // ヘッドホン音量 (0-31)

    OutputSettings(SettingsStore* store);

protected:
    void serializeItems(OutputArchive& archive) const override;
    void deserializeItems(InputArchive& archive) override;
};

// 表示設定
class DisplaySettings : public SettingsCategoryBase {
public:
    SettingDescriptor<uint8_t> brightness;  // 画面の明るさ (0:Bright, 1:Normal, 2:Dark)

    DisplaySettings(SettingsStore* store);

protected:
    void serializeItems(OutputArchive& archive) const override;
    void deserializeItems(InputArchive& archive) override;
};

// システム設定
class SystemSettings : public SettingsCategoryBase {
public:
    SettingDescriptor<bool> airplaneMode;  // 機内モード (スタブ)
    SettingDescriptor<bool> silentMode;    // サイレントモード (スタブ)

    SystemSettings(SettingsStore* store);

protected:
    void serializeItems(OutputArchive& archive) const override;
    void deserializeItems(InputArchive& archive) override;
};

// 設定ストア（シングルトン）
class SettingsStore {
private:
    static SettingsStore* instance;
    portMUX_TYPE listenerMutex = portMUX_INITIALIZER_UNLOCKED;
    std::list<SettingsListener*> globalListeners;
    std::vector<SettingsCategoryBase*> categories;

    SettingsStore();

public:
    // シングルトンアクセス
    static SettingsStore& getInstance();

    // コピー/ムーブ禁止
    SettingsStore(const SettingsStore&) = delete;
    SettingsStore& operator=(const SettingsStore&) = delete;

    // カテゴリ
    ControlsSettings controls;       // 操作
    PerformanceSettings performance; // 演奏
    VoicingSettings voicing;         // ボイシング
    OutputSettings output;           // 出力
    DisplaySettings display;         // 表示
    SystemSettings system;           // システム

    // グローバルリスナー管理
    void addListener(SettingsListener* listener);
    void removeListener(SettingsListener* listener);

    // グローバルリスナーへの通知（SettingDescriptorから呼び出される）
    void notifyGlobalListeners(const char* key);

    // 永続化
    bool loadAll();      // 全カテゴリを読み込み
    bool saveAll();      // 全カテゴリを保存
    void saveIfDirty();  // 変更のあったカテゴリのみ保存

    // 旧設定ファイルからの移行
    bool migrateFromLegacy();
};

// グローバルアクセスマクロ
#define Settings SettingsStore::getInstance()

// SettingDescriptor::serialize/deserialize の実装
// 一般的な型用
template<typename T>
void SettingDescriptor<T>::serialize(OutputArchive& archive) const {
    archive(key, value);
}

template<typename T>
void SettingDescriptor<T>::deserialize(InputArchive& archive) {
    archive(key, value);
}

// Scale 用の特殊化
template<>
inline void SettingDescriptor<Scale>::serialize(OutputArchive& archive) const {
    value.serialize(archive, key);
}

template<>
inline void SettingDescriptor<Scale>::deserialize(InputArchive& archive) {
    value.deserialize(archive, key);
}

// SettingDescriptor::set の実装（SettingsStoreの定義後に必要）
template<typename T>
void SettingDescriptor<T>::set(const T& newValue) {
    T oldValue;
    std::list<std::function<void(const T&, const T&)>> listenersCopy;

    // クリティカルセクション: 値の更新とリスナーのコピー
    portENTER_CRITICAL(&mutex);
    if (value == newValue) {
        portEXIT_CRITICAL(&mutex);
        return; // 変更なし
    }
    oldValue = value;
    value = newValue;
    listenersCopy = listeners;
    portEXIT_CRITICAL(&mutex);

    // dirtyフラグをセット
    if (categoryDirtyFlag) {
        *categoryDirtyFlag = true;
    }

    // ローカルリスナーへの通知（クリティカルセクション外で実行）
    for (auto& listener : listenersCopy) {
        listener(oldValue, newValue);
    }

    // グローバルリスナーへの通知
    if (store) {
        store->notifyGlobalListeners(key);
    }
}

#endif // _SETTINGS_STORE_H_
