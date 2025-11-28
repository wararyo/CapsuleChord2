#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "Settings.h"
#include "Chord.h"
#include "ChordPipeline.h"
#include "Keypad.h"
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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
    CapsuleChordKeypad *keypad;

    Context() : pipeline(nullptr), keypad(nullptr) {
        knockMutex = xSemaphoreCreateMutex();
        if (!knockMutex) {
            Serial.println("FATAL: Failed to create Context knockMutex");
        }
    } // デフォルトコンストラクタ(通常は使用しない)

    // 通常はこちらのコンストラクタを使用する
    Context(Settings *settings, ChordPipeline *pipeline, CapsuleChordKeypad *keypad)
        : settings(settings), pipeline(pipeline), keypad(keypad)
    {
        knockMutex = xSemaphoreCreateMutex();
        if (!knockMutex) {
            Serial.println("FATAL: Failed to create Context knockMutex");
        }
        scale = &((SettingItemScale *)settings->findSettingByKey(String("Scale")))->content;
        centerNoteNo = &((SettingItemNumeric *)settings->findSettingByKey(String("CenterNoteNo")))->number;
    }

    ~Context() {
        if (knockMutex) {
            vSemaphoreDelete(knockMutex);
            knockMutex = nullptr;
        }
    }

    // Register a listener for knock events
    void addKnockListener(KnockListener* listener) {
        if (listener && knockMutex) {
            xSemaphoreTake(knockMutex, portMAX_DELAY);
            knockListeners.push_back(listener);
            xSemaphoreGive(knockMutex);
        }
    }

    // Unregister a listener for knock events
    void removeKnockListener(KnockListener* listener) {
        if (!knockMutex) return;
        xSemaphoreTake(knockMutex, portMAX_DELAY);
        for (auto it = knockListeners.begin(); it != knockListeners.end(); ++it) {
            if (*it == listener) {
                knockListeners.erase(it);
                break;
            }
        }
        xSemaphoreGive(knockMutex);
    }

    // Send a knock event to all registered listeners
    void knock(AppBase* app) {
        if (!app || !knockMutex) return;

        xSemaphoreTake(knockMutex, portMAX_DELAY);
        // Copy to avoid issues if listener modifies the list
        auto listenersCopy = knockListeners;
        xSemaphoreGive(knockMutex);

        for (auto listener : listenersCopy) {
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
    SemaphoreHandle_t knockMutex = nullptr;
};

#endif