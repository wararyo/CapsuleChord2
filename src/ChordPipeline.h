#pragma once
#include <cstdint>
#include <vector>
#include <list>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "Chord.h"

static const char* LOG_TAG_CHORD_PIPELINE = "ChordPipeline";

// キーを押してからMIDIを出力するまでの一連の動作を管理するクラス
//
// スレッド安全性:
// - 公開メソッドはどのタスクから呼んでもよい。内部は再帰ミューテックスで直列化される
//   (I2Cタスクのキー入力、TempoTickタスクのonTick、メインループのUI操作が同時に入ってくる)
// - フィルターのコールバックはこのロックを保持したまま呼ばれる。
//   コールバック内でブロッキング処理 (ファイルI/O、LVGL操作、長いI2C通信など) をしないこと
// - コールバックが触る状態を別タスクから読み書きする場合は ChordPipeline::Lock を取ること
// - ISRやportENTER_CRITICALの区間内から呼ばないこと (ミューテックスを取得するため)
// - コールバック内からフィルターの登録・解除をしないこと (反復中のイテレータが無効になる)
class ChordPipeline
{
public:
    ChordPipeline() {
        // フィルターがコールバック内でplayChord/sendNoteを呼び直すため、再帰ミューテックスにする
        pipelineMutex = xSemaphoreCreateRecursiveMutex();
        if (!pipelineMutex) {
            ESP_LOGE(LOG_TAG_CHORD_PIPELINE, "FATAL: Failed to create ChordPipeline pipelineMutex");
        }
    }

    ~ChordPipeline() {
        if (pipelineMutex) {
            vSemaphoreDelete(pipelineMutex);
            pipelineMutex = nullptr;
        }
    }

    // パイプラインのロックを取得するRAIIヘルパー
    // フィルターのコールバックと共有する状態を別タスクから触る際にも使う
    class Lock
    {
    public:
        explicit Lock(ChordPipeline &pipeline) : mutex(pipeline.pipelineMutex)
        {
            if (!mutex) return;
            // デッドロック調査用: 長時間取得できない場合は待機中のタスク名を出しつつ待ち続ける
            while (xSemaphoreTakeRecursive(mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
                ESP_LOGE(LOG_TAG_CHORD_PIPELINE, "Pipeline lock wait > 1s (task=%s)", pcTaskGetName(nullptr));
            }
        }
        ~Lock()
        {
            if (mutex) xSemaphoreGiveRecursive(mutex);
        }
        Lock(const Lock&) = delete;
        Lock& operator=(const Lock&) = delete;
    private:
        SemaphoreHandle_t mutex;
    };

    // コピー・ムーブ禁止（mutexの二重解放を防ぐ）
    ChordPipeline(const ChordPipeline&) = delete;
    ChordPipeline& operator=(const ChordPipeline&) = delete;
    ChordPipeline(ChordPipeline&&) = delete;
    ChordPipeline& operator=(ChordPipeline&&) = delete;

    // 鳴っているコードの変更を受け取ったり変更するためのフィルター
    class ChordFilter
    {
    public:
        // ChordPipeline外からは使用しない
        void setHandler(std::list<ChordFilter *>::iterator handler)
        {
            this->handler = handler;
        }
        std::list<ChordFilter *>::iterator getHandler()
        {
            return handler;
        }
        // このフィルターがコードを変更しうるかどうか
        // trueにする場合はフィルター側でplayChordを呼び出す、その際にfilterを指定する
        // falseにする場合はplayChordを呼び出さないこと！(二重にコードが鳴る)
        virtual bool modifiesChord() { return false; }
        // コードが鳴り始めたときに呼び出される
        virtual void onChordOn(Chord chord) = 0;
        virtual void onChordOff() = 0;
    private:
        friend class ChordPipeline;
        std::list<ChordFilter *>::iterator handler;
        bool registered = false; // ChordPipelineのロック下でのみ読み書きする
    };

    // 鳴っているノートの変更を受け取ったり変更するためのフィルター
    class NoteFilter
    {
    public:
        // ChordPipeline外からは使用しない
        void setHandler(std::list<NoteFilter *>::iterator handler)
        {
            this->handler = handler;
        }
        std::list<NoteFilter *>::iterator getHandler()
        {
            return handler;
        }
        // このフィルターがノートを変更しうるかどうか
        // trueにする場合はフィルター側でsendNoteを呼び出す、その際にfilterを指定する
        // falseにする場合はsendNoteを呼び出さないこと！(二重に音が鳴る)
        virtual bool modifiesNote() { return false; }
        // ノートが鳴り始めたときに呼び出される
        virtual void onNoteOn(uint8_t note, uint8_t vel, uint8_t channel) = 0;
        virtual void onNoteOff(uint8_t note, uint8_t vel, uint8_t channel) = 0;
    private:
        friend class ChordPipeline;
        std::list<NoteFilter *>::iterator handler;
        bool registered = false; // ChordPipelineのロック下でのみ読み書きする
    };

    void sendNote(bool isNoteOn, uint8_t note, uint8_t vel, uint8_t channel = 0, NoteFilter *filter = nullptr);
    void sendNotes(bool isNoteOn, std::vector<uint8_t> notes, uint8_t vel, uint8_t channel = 0, NoteFilter *filter = nullptr);
    void playChord(Chord chord, ChordFilter *filter = nullptr);
    void stopChord(ChordFilter *filter = nullptr);
    // 登録済みのフィルターを再度登録しても何もしない
    void addChordFilter(ChordFilter *filter)
    {
        Lock lock(*this);
        if (filter->registered) return;
        filter->setHandler(chordFilters.insert(chordFilters.end(), filter));
        filter->registered = true;
    }
    void removeChordFilter(ChordFilter *filter)
    {
        Lock lock(*this);
        if (!filter->registered) return;
        chordFilters.erase(filter->getHandler());
        filter->registered = false;
    }
    // 登録済みのフィルターを再度登録しても何もしない
    void addNoteFilter(NoteFilter *filter)
    {
        Lock lock(*this);
        if (filter->registered) return;
        filter->setHandler(noteFilters.insert(noteFilters.end(), filter));
        filter->registered = true;
    }
    void removeNoteFilter(NoteFilter *filter)
    {
        Lock lock(*this);
        if (!filter->registered) return;
        noteFilters.erase(filter->getHandler());
        filter->registered = false;
    }

private:
    // chordFilters / noteFilters / playingNotes はすべてこのミューテックスで保護する
    std::list<ChordFilter *> chordFilters;
    std::list<NoteFilter *> noteFilters;
    std::vector<uint8_t> playingNotes;
    SemaphoreHandle_t pipelineMutex = nullptr;

    void sendNoteOnRaw(uint8_t note, uint8_t vel, uint8_t channel);
    void sendNoteOffRaw(uint8_t note, uint8_t vel, uint8_t channel);

    // 呼び出し元として指定されたフィルターが登録解除済みかどうか
    // 登録解除済みのフィルターからは次のフィルターを辿れないため、呼び出しを破棄させる
    // (例: removeListener後も走り切るonTickが、removeNoteFilter済みのフィルターを指定してsendNoteする)
    template <typename Filter>
    static bool isUnregisteredFilter(Filter *filter, const char *caller)
    {
        if (filter == nullptr || filter->registered) return false;
        ESP_LOGW(LOG_TAG_CHORD_PIPELINE, "%s: called with an unregistered filter, dropped", caller);
        return true;
    }
};

extern ChordPipeline Pipeline;
