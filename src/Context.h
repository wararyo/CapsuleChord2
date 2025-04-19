#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "Settings.h"
#include "Chord.h"
#include "ChordPipeline.h"
#include <vector>

// Forward declaration for AppBase
class AppBase;

class Context
{
public:
    // KnockListener interface for receiving knock events
    class KnockListener {
    public:
        virtual void onKnock(AppBase* app) = 0;
        virtual ~KnockListener() = default;
    };

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

    // Register a listener for knock events
    void addKnockListener(KnockListener* listener) {
        if (listener) {
            knockListeners.push_back(listener);
        }
    }

    // Unregister a listener for knock events
    void removeKnockListener(KnockListener* listener) {
        for (auto it = knockListeners.begin(); it != knockListeners.end(); ++it) {
            if (*it == listener) {
                knockListeners.erase(it);
                break;
            }
        }
    }

    // Send a knock event to all registered listeners
    void knock(AppBase* app) {
        if (!app) return;
        
        for (auto listener : knockListeners) {
            listener->onKnock(app);
        }
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
    std::vector<KnockListener*> knockListeners;
};

#endif