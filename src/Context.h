#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "Settings.h"
#include "Chord.h"
#include "ChordPipeline.h"

class Context
{
public:
    Settings *settings;
    Scale *scale;
    int *centerNoteNo;
    ChordPipeline *pipeline;

    Context() : pipeline(nullptr) {} // デフォルトコンストラクタ(通常は使用しない)

    // 通常はこちらのコンストラクタを使用する
    Context(Settings *settings, ChordPipeline *pipeline)
        : settings(settings), pipeline(pipeline)
    {
        scale = &((SettingItemScale *)settings->findSettingByKey(String("Scale")))->content;
        centerNoteNo = &((SettingItemNumeric *)settings->findSettingByKey(String("CenterNoteNo")))->number;
    }

    static Context *getContext()
    {
        if (_instance == nullptr)
            _instance = new Context();
        return _instance;
    }

    static void setContext(Context *context)
    {
        _instance = context;
    }

protected:
    static Context *_instance;
};

#endif