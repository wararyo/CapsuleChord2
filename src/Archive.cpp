#include <Archive.h>

//const char *
void serialize(OutputArchive &archive,const char *key,const char *string){
    archive.getDocument()[key] = string;
}
void deserialize(InputArchive &archive,const char *key,const char*& string){
    if(archive.getDocument().containsKey(key)) {
        string = archive.getDocument()[key];
    }
}

//std::string
void serialize(OutputArchive &archive,const char *key,const std::string& string){
    archive.getDocument()[key] = string.c_str();
}
void deserialize(InputArchive &archive,const char *key,std::string& string){
    if(archive.getDocument().containsKey(key)) {
        const char* str = archive.getDocument()[key];
        string = std::string(str);
    }
}

//int
void serialize(OutputArchive &archive,const char *key,int number){
    archive.getDocument()[key] = number;
}
void deserialize(InputArchive &archive,const char *key,int& number){
    if(archive.getDocument().containsKey(key)) number = archive.getDocument()[key];
}

//uint
void serialize(OutputArchive &archive,const char *key,uint number){
    archive.getDocument()[key] = number;
}
void deserialize(InputArchive &archive,const char *key,uint& number){
    if(archive.getDocument().containsKey(key)) number = archive.getDocument()[key];
}

//uint8_t
void serialize(OutputArchive &archive,const char *key,uint8_t number){
    archive.getDocument()[key] = number;
}
void deserialize(InputArchive &archive,const char *key,uint8_t& number){
    if(archive.getDocument().containsKey(key)) number = archive.getDocument()[key];
}

//uint16_t
void serialize(OutputArchive &archive,const char *key,uint16_t number){
    archive.getDocument()[key] = number;
}
void deserialize(InputArchive &archive,const char *key,uint16_t& number){
    if(archive.getDocument().containsKey(key)) number = archive.getDocument()[key];
}

//float
void serialize(OutputArchive &archive,const char *key,float number){
    archive.getDocument()[key] = number;
}
void deserialize(InputArchive &archive,const char *key,float& number){
    if(archive.getDocument().containsKey(key)) number = archive.getDocument()[key];
}

//bool
void serialize(OutputArchive &archive,const char *key,bool value){
    archive.getDocument()[key] = value;
}
void deserialize(InputArchive &archive,const char *key,bool& value){
    if(archive.getDocument().containsKey(key)) value = archive.getDocument()[key];
}