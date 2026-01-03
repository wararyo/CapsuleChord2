#include "SettingsStore.h"
#include <sys/stat.h>
#include <cstring>

// シングルトンインスタンス
SettingsStore* SettingsStore::instance = nullptr;

// SettingsCategoryBase の共通実装
bool SettingsCategoryBase::load() {
    if (!isLittleFSMounted()) {
        ESP_LOGE(LOG_TAG_SETTINGS_STORE, "LittleFS not mounted");
        return false;
    }

    std::string fullPath = std::string(LITTLEFS_MOUNT_POINT) + filePath;
    FILE* file = fopen(fullPath.c_str(), "r");
    if (!file) {
        ESP_LOGW(LOG_TAG_SETTINGS_STORE, "Settings file not found: %s", fullPath.c_str());
        return false;
    }

    // ファイルサイズを取得
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // ファイル内容を読み込む
    std::vector<char> buffer(fileSize + 1);
    size_t bytesRead = fread(buffer.data(), 1, fileSize, file);
    buffer[bytesRead] = '\0';
    fclose(file);

    // JSONをデシリアライズ
    InputArchive archive;
    archive.fromJSON(buffer.data());
    deserializeItems(archive);

    isDirty = false;
    ESP_LOGI(LOG_TAG_SETTINGS_STORE, "Loaded: %s", fullPath.c_str());
    return true;
}

bool SettingsCategoryBase::save() {
    if (!isLittleFSMounted()) {
        ESP_LOGE(LOG_TAG_SETTINGS_STORE, "LittleFS not mounted");
        return false;
    }

    // /settings ディレクトリを作成（存在しなければ）
    std::string dirPath = std::string(LITTLEFS_MOUNT_POINT) + "/settings";
    struct stat st;
    if (stat(dirPath.c_str(), &st) != 0) {
        if (mkdir(dirPath.c_str(), 0755) != 0) {
            ESP_LOGE(LOG_TAG_SETTINGS_STORE, "Failed to create directory: %s", dirPath.c_str());
            return false;
        }
    }

    std::string fullPath = std::string(LITTLEFS_MOUNT_POINT) + filePath;

    // JSONにシリアライズ
    OutputArchive archive;
    serializeItems(archive);
    std::string json = archive.toJSON(true);

    // ファイルに書き込み
    FILE* file = fopen(fullPath.c_str(), "w");
    if (!file) {
        ESP_LOGE(LOG_TAG_SETTINGS_STORE, "Failed to open for writing: %s", fullPath.c_str());
        return false;
    }

    size_t written = fwrite(json.c_str(), 1, json.length(), file);
    fclose(file);

    if (written != json.length()) {
        ESP_LOGE(LOG_TAG_SETTINGS_STORE, "Write failed: %s", fullPath.c_str());
        return false;
    }

    isDirty = false;
    ESP_LOGI(LOG_TAG_SETTINGS_STORE, "Saved: %s", fullPath.c_str());
    return true;
}

// ControlsSettings
ControlsSettings::ControlsSettings(SettingsStore* store)
    : SettingsCategoryBase("/settings/controls.json"),
      customKey1("customKey1", 0, &isDirty, store),
      customKey2("customKey2", 0, &isDirty, store),
      leftTrigger("leftTrigger", 0, &isDirty, store),
      rightTrigger("rightTrigger", 1, &isDirty, store) {}

void ControlsSettings::serializeItems(OutputArchive& archive) const {
    customKey1.serialize(archive);
    customKey2.serialize(archive);
    leftTrigger.serialize(archive);
    rightTrigger.serialize(archive);
}

void ControlsSettings::deserializeItems(InputArchive& archive) {
    customKey1.deserialize(archive);
    customKey2.deserialize(archive);
    leftTrigger.deserialize(archive);
    rightTrigger.deserialize(archive);
}

// PerformanceSettings
PerformanceSettings::PerformanceSettings(SettingsStore* store)
    : SettingsCategoryBase("/settings/performance.json"),
      scale("scale", Scale(0), &isDirty, store) {}

void PerformanceSettings::serializeItems(OutputArchive& archive) const {
    scale.serialize(archive);
}

void PerformanceSettings::deserializeItems(InputArchive& archive) {
    scale.deserialize(archive);
}

// VoicingSettings
VoicingSettings::VoicingSettings(SettingsStore* store)
    : SettingsCategoryBase("/settings/voicing.json"),
      centerNoteNo("centerNoteNo", 60, &isDirty, store) {}

void VoicingSettings::serializeItems(OutputArchive& archive) const {
    centerNoteNo.serialize(archive);
}

void VoicingSettings::deserializeItems(InputArchive& archive) {
    centerNoteNo.deserialize(archive);
}

// OutputSettings
OutputSettings::OutputSettings(SettingsStore* store)
    : SettingsCategoryBase("/settings/output.json"),
      outputTarget("outputTarget", 0, &isDirty, store),
      speakerVolume("speakerVolume", 16, &isDirty, store),
      headphoneVolume("headphoneVolume", 16, &isDirty, store) {}

void OutputSettings::serializeItems(OutputArchive& archive) const {
    outputTarget.serialize(archive);
    speakerVolume.serialize(archive);
    headphoneVolume.serialize(archive);
}

void OutputSettings::deserializeItems(InputArchive& archive) {
    outputTarget.deserialize(archive);
    speakerVolume.deserialize(archive);
    headphoneVolume.deserialize(archive);
}

// DisplaySettings
DisplaySettings::DisplaySettings(SettingsStore* store)
    : SettingsCategoryBase("/settings/display.json"),
      brightness("brightness", 1, &isDirty, store) {}  // 1 = Normal

void DisplaySettings::serializeItems(OutputArchive& archive) const {
    brightness.serialize(archive);
}

void DisplaySettings::deserializeItems(InputArchive& archive) {
    brightness.deserialize(archive);
}

// SystemSettings
SystemSettings::SystemSettings(SettingsStore* store)
    : SettingsCategoryBase("/settings/system.json"),
      airplaneMode("airplaneMode", false, &isDirty, store),
      silentMode("silentMode", false, &isDirty, store) {}

void SystemSettings::serializeItems(OutputArchive& archive) const {
    airplaneMode.serialize(archive);
    silentMode.serialize(archive);
}

void SystemSettings::deserializeItems(InputArchive& archive) {
    airplaneMode.deserialize(archive);
    silentMode.deserialize(archive);
}

// SettingsStore
SettingsStore::SettingsStore()
    : controls(this),
      performance(this),
      voicing(this),
      output(this),
      display(this),
      system(this) {
    // カテゴリをベクターに登録
    categories.push_back(&controls);
    categories.push_back(&performance);
    categories.push_back(&voicing);
    categories.push_back(&output);
    categories.push_back(&display);
    categories.push_back(&system);
}

SettingsStore& SettingsStore::getInstance() {
    if (instance == nullptr) {
        instance = new SettingsStore();
    }
    return *instance;
}

void SettingsStore::addListener(SettingsListener* listener) {
    portENTER_CRITICAL(&listenerMutex);
    globalListeners.push_back(listener);
    portEXIT_CRITICAL(&listenerMutex);
}

void SettingsStore::removeListener(SettingsListener* listener) {
    portENTER_CRITICAL(&listenerMutex);
    globalListeners.remove(listener);
    portEXIT_CRITICAL(&listenerMutex);
}

void SettingsStore::notifyGlobalListeners(const char* key) {
    // リスナーのコピーを取得（デッドロック回避）
    portENTER_CRITICAL(&listenerMutex);
    std::list<SettingsListener*> listenersCopy = globalListeners;
    portEXIT_CRITICAL(&listenerMutex);

    for (auto listener : listenersCopy) {
        listener->onSettingChanged(key);
    }
}

bool SettingsStore::loadAll() {
    bool allSuccess = true;
    for (auto category : categories) {
        if (!category->load()) {
            // ファイルがなければデフォルト値で保存
            if (!category->save()) {
                allSuccess = false;
            }
        }
    }
    return allSuccess;
}

bool SettingsStore::saveAll() {
    bool allSuccess = true;
    for (auto category : categories) {
        if (!category->save()) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

void SettingsStore::saveIfDirty() {
    for (auto category : categories) {
        if (category->needsSave()) {
            category->save();
        }
    }
}

bool SettingsStore::migrateFromLegacy() {
    if (!isLittleFSMounted()) {
        return false;
    }

    std::string legacyPath = std::string(LITTLEFS_MOUNT_POINT) + "/settings.json";
    FILE* file = fopen(legacyPath.c_str(), "r");
    if (!file) {
        // 旧ファイルなし - 移行不要
        return true;
    }

    ESP_LOGI(LOG_TAG_SETTINGS_STORE, "Migrating from legacy settings.json");

    // ファイルサイズを取得
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // ファイル内容を読み込む
    std::vector<char> buffer(fileSize + 1);
    size_t bytesRead = fread(buffer.data(), 1, fileSize, file);
    buffer[bytesRead] = '\0';
    fclose(file);

    // JSONをパース
    InputArchive archive;
    archive.fromJSON(buffer.data());

    // 旧設定を読み込み
    Scale legacyScale(0);
    int legacyCenterNoteNo = 60;
    uint8_t legacyBrightness = 1;

    archive("Scale", legacyScale);
    archive("CenterNoteNo", legacyCenterNoteNo);
    archive("Brightness", legacyBrightness);

    // 新設定に反映
    performance.scale.setValueWithoutNotify(legacyScale);
    voicing.centerNoteNo.setValueWithoutNotify(legacyCenterNoteNo);
    display.brightness.setValueWithoutNotify(legacyBrightness);

    // 新形式で保存
    saveAll();

    // 旧ファイルを削除
    remove(legacyPath.c_str());

    ESP_LOGI(LOG_TAG_SETTINGS_STORE, "Migration complete");
    return true;
}
