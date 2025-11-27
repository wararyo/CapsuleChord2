#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

#include <ArduinoJson.h>
#undef min
#include <vector>
#include <string>
#include <memory>
#include <stdio.h>

#define AUTO_NVP(T) #T, T

const int maxJsonCapacity = JSON_OBJECT_SIZE(64);
const int maxJsonFileSize = 2048;

/* Meta functions */

class OutputArchive;
class InputArchive;

struct is_serializable_internally_impl {
    template <class T>
    static auto check(OutputArchive oa,InputArchive ia, const char * name,T&& x)->decltype(x.serialize(oa,name),x.deserialize(ia,name),std::true_type{});
    template <class T>
    static auto check(...)->std::false_type;
};
struct is_serializable_externally_impl {
    template <class T>
    static auto check(OutputArchive oa,InputArchive ia, const char * name,T&& x)->decltype(serialize(oa,name,std::forward<T>(x)),deserialize(ia,name,std::forward<T>(x)),std::true_type{});
    template <class T>
    static auto check(...)->std::false_type;
};
struct is_serializable_impl {
    template <class T>
    static auto check(OutputArchive oa,InputArchive ia, const char * name,T&& x)->decltype(x.serialize(oa,name),x.deserialize(ia,name),std::true_type{});
    template <class T>
    static auto check(OutputArchive oa,InputArchive ia, const char * name,T&& x)->decltype(serialize(oa,name,std::forward<T>(x)),deserialize(ia,name,std::forward<T>(x)),std::true_type{});
    template <class T>
    static auto check(...)->std::false_type;
};

// constオブジェクト用
struct is_output_serializable_internally_impl {
    template <class T>
    static auto check(OutputArchive oa, const char * name, const T& x)->decltype(x.serialize(oa,name),std::true_type{});
    template <class T>
    static auto check(...)->std::false_type;
};
struct is_output_serializable_externally_impl {
    template <class T>
    static auto check(OutputArchive oa, const char * name, const T& x)->decltype(serialize(oa,name,x),std::true_type{});
    template <class T>
    static auto check(...)->std::false_type;
};

// Serializable internally or externally
template <class T>
class is_serializable :
    public decltype(is_serializable_impl::check<T>(std::declval<OutputArchive>(), std::declval<InputArchive>(), std::declval<const char *>(), std::declval<T>())){};

template <class T>
class is_serializable_internally :
    public decltype(is_serializable_internally_impl::check<T>(std::declval<OutputArchive>(), std::declval<InputArchive>(), std::declval<const char *>(), std::declval<T>())){};
template <class T>
class is_serializable_externally :
    public decltype(is_serializable_externally_impl::check<T>(std::declval<OutputArchive>(), std::declval<InputArchive>(), std::declval<const char *>(), std::declval<T>())){};

// constオブジェクト用
template <class T>
class is_output_serializable_internally :
    public decltype(is_output_serializable_internally_impl::check<T>(std::declval<OutputArchive>(), std::declval<const char *>(), std::declval<const T>())){};
template <class T>
class is_output_serializable_externally :
    public decltype(is_output_serializable_externally_impl::check<T>(std::declval<OutputArchive>(), std::declval<const char *>(), std::declval<const T>())){};

/* Archive class */

class OutputArchive {
private:
    std::vector<JsonObject> nestStack;
    DynamicJsonDocument doc = DynamicJsonDocument(maxJsonCapacity);
public:
    OutputArchive() {
        nestStack.push_back(doc.to<JsonObject>());
    }
    // 通常はこちらが使用される
    template <class T, typename std::enable_if<is_serializable_internally<T>::value, std::nullptr_t>::type = nullptr>
    inline void operator()(const char *key,T && arg) {
        arg.serialize(*this,key);
    }
    template <class T, typename std::enable_if<is_serializable_externally<T>::value, std::nullptr_t>::type = nullptr>
    inline void operator()(const char *key,T && arg) {
        serialize(*this,key,std::forward<T>(arg));
    }
    // 出力(シリアライズ)のみ、constオブジェクトを受け付けるオーバーロードを用意
    template <class T, typename std::enable_if<is_output_serializable_internally<T>::value, std::nullptr_t>::type = nullptr>
    inline void operator()(const char *key, const T& arg) {
        arg.serialize(*this,key);
    }
    template <class T, typename std::enable_if<is_output_serializable_externally<T>::value, std::nullptr_t>::type = nullptr>
    inline void operator()(const char *key, const T& arg) {
        serialize(*this,key,arg);
    }

    void pushNest(const char *key) {
        nestStack.push_back(nestStack[nestStack.size()-1].createNestedObject(key));
    }
    void popNest() {
        if(!nestStack.empty()) nestStack.pop_back();
    }
    JsonObject getDocument(){
        return nestStack[nestStack.size()-1];
    }
    std::string toJSON(bool isPretty = false){
        std::string output;
        if(isPretty) {
            output.reserve(maxJsonFileSize);
            serializeJsonPretty(doc, output);
        }
        else {
            output.reserve(maxJsonFileSize);
            serializeJson(doc, output);
        }
        return output;
    }
};

class InputArchive {
private:
    std::vector<JsonObject> nestStack;
    DynamicJsonDocument doc = DynamicJsonDocument(maxJsonCapacity);
public:
    InputArchive() {
        nestStack.push_back(doc.to<JsonObject>());
    }
    template <class T, typename std::enable_if<is_serializable_internally<T>::value, std::nullptr_t>::type = nullptr>
    inline void operator()(const char *key,T && arg) { 
        arg.deserialize(*this,key);
    }
    template <class T, typename std::enable_if<is_serializable_externally<T>::value, std::nullptr_t>::type = nullptr>
    inline void operator()(const char *key,T && arg) {
        deserialize(*this,key,std::forward<T>(arg));
    }
    bool pushNest(const char *key) {
        JsonObject top = nestStack[nestStack.size()-1];
        if(top.containsKey(key)){
            JsonObject obj = top[key];
            if(obj.size() > 0) {
                nestStack.push_back(obj);
                return true;
            }
        }
        return false;
    }
    void popNest() {
        if(!nestStack.empty()) nestStack.pop_back();
    }
    JsonObject getDocument(){
        return nestStack[nestStack.size()-1];
    }
    void fromJSON(const char *in){
        deserializeJson(doc,in);
    }
};


/* Serialization of Types */

//const char *
void serialize(OutputArchive &archive,const char *key,const char *string);
void deserialize(InputArchive &archive,const char *key,const char * && string);

//String
void serialize(OutputArchive &archive,const char *key,String string);
void deserialize(InputArchive &archive,const char *key,String && string);

//int
void serialize(OutputArchive &archive,const char *key,int number);
void deserialize(InputArchive &archive,const char *key,int && number);

//uint
void serialize(OutputArchive &archive,const char *key,uint number);
void deserialize(InputArchive &archive,const char *key,uint && number);

//uint8_t
void serialize(OutputArchive &archive,const char *key,uint8_t number);
void deserialize(InputArchive &archive,const char *key,uint8_t && number);

//char
void serialize(OutputArchive &archive,const char *key,char number);
void deserialize(InputArchive &archive,const char *key,char && number);

//float
void serialize(OutputArchive &archive,const char *key,float number);
void deserialize(InputArchive &archive,const char *key,float && number);

//bool
void serialize(OutputArchive &archive,const char *key,bool value);
void deserialize(InputArchive &archive,const char *key,bool && value);

//std::vector
template <class T, class A>
void serialize(OutputArchive &archive,const char *key,const std::vector<T, A>& list){
    archive.pushNest(key);
    for(const auto& item : list) {
        archive("item", item);
    }
    archive.popNest();
}
template <class T, class A>
void deserialize(InputArchive &archive,const char *key,std::vector<T, A> && list){
    if(archive.pushNest(key)) {
        for(auto& item : list) {
            archive("item", item);
        }
        archive.popNest();
    }
}

//pointer
template <class T>
void serialize(OutputArchive &archive,const char *key,T* ptr){
    archive(key,*ptr);
}
template <class T>
void deserialize(InputArchive &archive,const char *key,T* ptr){
    archive(key,*ptr);
}

//std::unique_ptr
template <class T>
void serialize(OutputArchive &archive,const char *key,const std::unique_ptr<T>& ptr){
    archive(key,*ptr);
}
template <class T>
void deserialize(InputArchive &archive,const char *key,std::unique_ptr<T> & ptr){
    if(!ptr) ptr = std::make_unique<T>();
    archive(key,*ptr);
}

//std::shared_ptr
template <class T>
void serialize(OutputArchive &archive,const char *key,const std::shared_ptr<T>& ptr){
    archive(key,*ptr);
}
template <class T>
void deserialize(InputArchive &archive,const char *key,std::shared_ptr<T> & ptr){
    if(!ptr) ptr = std::make_shared<T>();
    archive(key,*ptr);
}

#endif