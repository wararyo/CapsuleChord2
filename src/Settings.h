#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "SD.h"
#undef min
#include <vector>
#include <memory>
#include <stdio.h>
#include "Archive.h"
#include "Chord.h"
#include "Scale.h"

#define MAX_NEST_SIZE 16

const String jsonFilePath = "/capsulechord/settings.json";

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
    bool load(String path = jsonFilePath){
        //Read file
        File file = SD.open(path);
        if(!file) return false;

        // Check file size to prevent buffer overflow
        size_t fileSize = file.size();
        if(fileSize >= maxJsonFileSize) {
            Serial.printf("Settings file too large: %zu bytes (max: %d)\n", fileSize, maxJsonFileSize - 1);
            file.close();
            return false;
        }

        char output[maxJsonFileSize] = {'\0'};
        size_t bytesRead = file.read((uint8_t *)output, fileSize);
        output[bytesRead] = '\0';  // Ensure null termination
        file.close();
        //Deserialize
        Serial.println("Start deserialization");
        InputArchive archive = InputArchive();
        archive.fromJSON(output);
        uint jsonVersion = 0;
        // archive("Version",std::forward<uint>(jsonVersion));
        Serial.printf("Setting file version = %d",jsonVersion);
        // if(version > jsonVersion) migration.invoke(jsonVersion);
        // archive(name,*this);
        return true;
    }
    bool save(String path = jsonFilePath){
        //Serialize
        OutputArchive archive = OutputArchive();
        // archive("Version",std::forward<uint>(version));
        // archive(name,*this);
        std::string output = archive.toJSON(true);
        //Write to file
        File file = SD.open(path,FILE_WRITE);
        if(!file) {
            Serial.println("Something wrong happened with saving settings.");
            return false;
        }
        file.print(output.c_str());
        file.close();
        return true;
    }
    SettingItem *findSettingByKey(String query){
        String keys[MAX_NEST_SIZE] = {String("\0")};
        // Split query by '/'
        int index = 0;
        for(int i = 0; i < query.length(); i++){
            char tmp = query.charAt(i);
            if(tmp == '/') {
                index++;
                if(index > (MAX_NEST_SIZE - 1)) return nullptr;
            }
            else keys[index] += tmp;
        }
        // Find item
        SettingItem * cursor = this;
        for(String key : keys){
            if(key == String("\0")) break;
            auto& children = cursor->children;
            for(auto& k : cursor->children){
                if(String(k->name) == key){
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
        String memberName = "";
        archive(name,memberName);
        for(int i = 0;i < memberNames.size();i++){
            String m = String(memberNames[i]);
            if(String(m).equals(memberName)) {
                index = i;
            }
        }
    }
};

#endif