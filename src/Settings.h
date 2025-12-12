#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <M5Unified.h>
#undef min
#include <vector>
#include <memory>
#include <stdio.h>
#include <esp_log.h>
#include "LittleFSManager.h"
#include "Archive.h"
#include "Chord.h"
#include "Scale.h"

static const char* LOG_TAG_SETTINGS = "Settings";

#define MAX_NEST_SIZE 16

// LittleFS上の設定ファイルパス（マウントポイントからの相対パス）
const std::string jsonFilePath = "/settings.json";

class SettingItem {
protected:
    // Scheme for executing member functions when the value of TreeView is changed
    static const int MaxTagCount = 128;
    static SettingItem* tags[MaxTagCount];
    int obtainTag() { //Tagを発行する
        for(int i = 0;i < MaxTagCount;i++) {
            if(tags[i] == nullptr){
                tags[i] = this;
                return i;
            }
        }
        return -1;
    }
    void clearTag(int tag){ //Tagを開放する
        if(tag >= 0 && tag < MaxTagCount) {
            tags[tag] = nullptr;
        }
    }
public:
    const char *name;
    std::vector<std::unique_ptr<SettingItem>> children;
    SettingItem(){}
    SettingItem(const char *name): name(name){}
    SettingItem(const char *name,std::vector<std::unique_ptr<SettingItem>> children)
        : name(name), children(std::move(children)) {}
    // Virtual destructor for proper cleanup
    virtual ~SettingItem() = default;
    virtual void serialize(OutputArchive &archive,const char *key) {
        if(children.empty()) {
            archive(name,"Empty Content");
        } else {
            archive(name,children);
        }
    }
    virtual void deserialize(InputArchive &archive,const char *key) {
        if(archive.getDocument().containsKey(key))
        {
            archive(name,children);
        }
    }
};

class Settings : public SettingItem {
private:
    uint version;

public:
    Settings(std::vector<std::unique_ptr<SettingItem>> items,uint version=1)
        : SettingItem("Settings",std::move(items)),version(version){}

    bool load(const std::string& path = jsonFilePath){
        if (!isLittleFSMounted()) {
            ESP_LOGE(LOG_TAG_SETTINGS, "LittleFS not mounted. Call mountLittleFS() first.");
            return false;
        }

        // VFSフルパスを構築
        std::string fullPath = std::string(LITTLEFS_MOUNT_POINT) + path;

        FILE* file = fopen(fullPath.c_str(), "r");
        if (file == nullptr) {
            ESP_LOGW(LOG_TAG_SETTINGS, "Settings file not found: %s", fullPath.c_str());
            return false;
        }

        // ファイルサイズを取得
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        // ファイル内容を読み込む
        std::vector<char> jsonBuffer(fileSize + 1);
        size_t bytesRead = fread(jsonBuffer.data(), 1, fileSize, file);
        jsonBuffer[bytesRead] = '\0';
        fclose(file);

        // JSONをデシリアライズ
        InputArchive archive;
        archive.fromJSON(jsonBuffer.data());

        // 各設定項目をデシリアライズ
        for (auto& item : children) {
            item->deserialize(archive, item->name);
        }

        ESP_LOGI(LOG_TAG_SETTINGS, "Settings loaded from: %s", fullPath.c_str());
        return true;
    }

    bool save(const std::string& path = jsonFilePath){
        if (!isLittleFSMounted()) {
            ESP_LOGE(LOG_TAG_SETTINGS, "LittleFS not mounted. Call mountLittleFS() first.");
            return false;
        }

        // VFSフルパスを構築
        std::string fullPath = std::string(LITTLEFS_MOUNT_POINT) + path;

        // JSONにシリアライズ
        OutputArchive archive;
        for (auto& item : children) {
            item->serialize(archive, item->name);
        }
        std::string json = archive.toJSON(true); // Pretty print

        // ファイルに書き込み
        FILE* file = fopen(fullPath.c_str(), "w");
        if (file == nullptr) {
            ESP_LOGE(LOG_TAG_SETTINGS, "Failed to open file for writing: %s", fullPath.c_str());
            return false;
        }

        size_t written = fwrite(json.c_str(), 1, json.length(), file);
        fclose(file);

        if (written != json.length()) {
            ESP_LOGE(LOG_TAG_SETTINGS, "Failed to write settings file");
            return false;
        }

        ESP_LOGI(LOG_TAG_SETTINGS, "Settings saved to: %s", fullPath.c_str());
        return true;
    }

    SettingItem *findSettingByKey(const std::string& query){
        std::string keys[MAX_NEST_SIZE] = {""};
        // Split query by '/'
        int index = 0;
        for(size_t i = 0; i < query.length(); i++){
            char tmp = query[i];
            if(tmp == '/') {
                index++;
                if(index > (MAX_NEST_SIZE - 1)) return nullptr;
            }
            else keys[index] += tmp;
        }
        // Find item
        SettingItem * cursor = this;
        for(const std::string& key : keys){
            if(key.empty()) break;
            auto& children = cursor->children;
            for(auto& k : cursor->children){
                if(std::string(k->name) == key){
                    cursor = k.get();
                    goto next_key; // forループを一気に抜けるための使用ならバチは当たらないはず…
                }
            }
            return nullptr;
next_key: ;
        }
        return cursor;
    }
};

class SettingItemDegreeChord : public SettingItem {
public:
    DegreeChord content;
    SettingItemDegreeChord(const char *name,DegreeChord content){
        this->name = name;
        this->content = content;
    }
    void serialize(OutputArchive &archive,const char *key) override {
        archive(name,content);
    }
    void deserialize(InputArchive &archive,const char *key) override {
        archive(name,content);
    }
};

class SettingItemScale : public SettingItem {
private:
    int keyTag;
    int scaleTag;
public:
    Scale content;
    SettingItemScale(const char *name,Scale content){
        this->name = name;
        this->content = content;
    }
    void serialize(OutputArchive &archive,const char *key) override {
        archive(name,content);
    }
    void deserialize(InputArchive &archive,const char *key) override {
        archive(name,content);
    }
};

class SettingItemBoolean : public SettingItem {
public:
    bool content;
    SettingItemBoolean(const char *name,bool content)
        : SettingItem(name), content(content) {}
    void serialize(OutputArchive &archive,const char *key) override {
        archive(name,content);
    }
    void deserialize(InputArchive &archive,const char *key) override {
        archive(name,content);
    }
};

class SettingItemNumeric : public SettingItem {
public:
    int number;
    int min;
    int max;
    SettingItemNumeric(const char *name,int min, int max, int number)
        : SettingItem(name), number(number), min(min), max(max) {}
    void serialize(OutputArchive &archive,const char *key) override {
        archive(name,number);
    }
    void deserialize(InputArchive &archive,const char *key) override {
        archive(name,number);
    }
};

class SettingItemEnum : public SettingItem {
public:
    std::vector<const char *> memberNames;
    uint8_t index;
    SettingItemEnum(const char *name, std::vector<const char *> memberNames, uint8_t index)
        : SettingItem(name), memberNames(memberNames), index(index) {}

    void serialize(OutputArchive &archive,const char *key) override {
        archive(name,memberNames[index]);
    }
    void deserialize(InputArchive &archive,const char *key) override {
        std::string memberName = "";
        archive(name,memberName);
        for(size_t i = 0;i < memberNames.size();i++){
            std::string m = std::string(memberNames[i]);
            if(m == memberName) {
                index = i;
            }
        }
    }
};

#endif
