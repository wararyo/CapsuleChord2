#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "Settings.h"
#include "Chord.h"

class Context {
public:
    Settings* settings;
    Scale* scale;
    int* centerNoteNo;
    void (*playChord)(Chord);
    void (*stopChord)();
    void (*sendNotes)(bool,std::vector<uint8_t>,int);

    Context(){}
    Context(Settings* settings):settings(settings){
        scale = &((SettingItemScale*)settings->findSettingByKey(String("Scale")))->content;
        centerNoteNo = &((SettingItemNumeric*)settings->findSettingByKey(String("CenterNoteNo")))->number;
    }

    static Context *getContext(){
        if(_instance == nullptr) _instance = new Context();
        return _instance;
    }
    static void setContext(Context *context) {
        _instance = context;
    }

protected:
    static Context *_instance; 
};

#endif