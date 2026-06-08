#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <list>
#include <cstdint>
#include "Foundation/MusicalTime.h"

// テンポの管理と通知を行うクラス
// 内部的にはウォールクロック (esp_timer_get_time) を一次ソースとして
// musical_time_t を都度導出する。これによりタイマーサービスタスクが遅延しても
// ドリフトせず、また再生中のテンポ変更もシームレスに反映される。
class TempoController
{
public:
    typedef uint16_t tick_timing_t;
    typedef uint16_t tempo_t;
    static const tick_timing_t TICK_TIMING_BAR          = 0b00000001; // 1小節
    static const tick_timing_t TICK_TIMING_FULL         = 0b00000010; // 1拍 (4/4においては4部音符、6/8においては8部音符1つ分)
    static const tick_timing_t TICK_TIMING_FULL_TRIPLET = 0b00000100; // 3連符
    static const tick_timing_t TICK_TIMING_HALF         = 0b00001000; // 1/2拍
    static const tick_timing_t TICK_TIMING_HALF_TRIPLET = 0b00010000; // 6連符
    static const tick_timing_t TICK_TIMING_QUARTER      = 0b00100000; // 1/4拍
    static const tick_timing_t TICK_TIMING_EIGHTH       = 0b10000000; // 1/8拍

    // 拍子
    struct TimeSignature
    {
        uint8_t num = 4;
        uint8_t denom = 4;
    };

    // Tick発火時にコールバックへ渡される情報
    struct TickInfo
    {
        tick_timing_t timing;      // 発火した種別のビットマスク
        musical_time_t time;       // 累積 musical_time (正確な境界値)
        musical_time_t timeInBar;  // 現在の小節内位置
        uint32_t bar;              // 0始まりの小節番号
    };

    // テンポの変更を通知するためのインターフェース
    // 各種コールバック内でTempo.start()やTempo.stop()を呼び出さない！(デッドロックが発生するため)
    class TempoCallbacks
    {
    public:
        virtual void onPlayingStateChanged(bool isPlaying) = 0;
        virtual void onTempoChanged(tempo_t tempo) = 0;
        virtual void onTick(const TickInfo &info) = 0;
        // Tickを通知するタイミング
        // tick_timing_tのビットフラグで指定する
        virtual tick_timing_t getTimingMask()
        {
            return TICK_TIMING_BAR;
        }
    };

    // テンポを取得する
    tempo_t getTempo() const
    {
        return tempo;
    }

    // カウント開始からの積算音楽時間を取得する (非再生時は0)
    musical_time_t getMusicalTime() const;

    // テンポを変更する (再生中でもシームレスに反映される)
    void setTempo(tempo_t newTempo);

    bool getPlaying() const
    {
        return isPlaying;
    }

    // Tick ディスパッチ用タスク／キューを初期化する (setup() から一度だけ呼ぶ)
    // play() 内でも未初期化なら遅延初期化されるため、明示呼び出しは必須ではない
    void begin();

    // テンポカウントを開始する
    void play();

    // テンポカウントを停止する
    void stop();

    // リスナーを追加する
    void addListener(TempoCallbacks *listener)
    {
        portENTER_CRITICAL(&mutex);
        listeners.push_back(listener);
        portEXIT_CRITICAL(&mutex);
    }

    // リスナーを削除する
    void removeListener(TempoCallbacks *listener)
    {
        portENTER_CRITICAL(&mutex);
        listeners.remove(listener);
        portEXIT_CRITICAL(&mutex);
    }

    // 現在の小節内位置
    musical_time_t timeInBar() const;
    // 任意時刻の小節内位置
    musical_time_t timeInBar(musical_time_t t) const;
    // 現在の小節番号 (0始まり)
    uint32_t getCurrentBar() const { return currentBar; }
    // 現在の拍子における1小節の長さ
    musical_time_t barLength() const;

private:
    bool isPlaying = false;
    mutable portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    tempo_t tempo = 110;
    std::list<TempoCallbacks *> listeners;
    TimerHandle_t timer = nullptr;

    // Tick の重い処理 (発音など) を Tmr Svc タスクから切り離すための仕組み。
    // timerWorkInner() は発火した TickInfo をキューへ積むだけにし、
    // 十分なスタックを持つ専用タスク (dispatchLoop) がキューを受けて onTick を実行する。
    QueueHandle_t tickQueue = nullptr;
    TaskHandle_t dispatchTask = nullptr;
    // 配送タスクが追従できずキューが溢れて破棄した tick の累計 (診断用)
    volatile uint32_t droppedTicks = 0;

    // ウォールクロックアンカー
    int64_t realTimeBase = 0;           // 最後のアンカー時の esp_timer_get_time() (μs)
    musical_time_t musicalTimeBase = 0; // 最後のアンカー時点での累積 musical_time

    // 各 tick 種別の最終発火位置 (musical_time_t)
    musical_time_t lastFiredMt[7] = {0};

    // 小節アンカー
    musical_time_t barAnchor = 0;
    uint32_t currentBar = 0;
    TimeSignature currentSig;

    // 各 tick 種別のインターバル (musical_time_t)
    static constexpr musical_time_t INTERVAL_FULL         = 480;
    static constexpr musical_time_t INTERVAL_FULL_TRIPLET = 160;
    static constexpr musical_time_t INTERVAL_HALF         = 240;
    static constexpr musical_time_t INTERVAL_HALF_TRIPLET = 80;
    static constexpr musical_time_t INTERVAL_QUARTER      = 120;
    static constexpr musical_time_t INTERVAL_EIGHTH       = 60;

    static constexpr size_t IDX_BAR          = 0;
    static constexpr size_t IDX_FULL         = 1;
    static constexpr size_t IDX_FULL_TRIPLET = 2;
    static constexpr size_t IDX_HALF         = 3;
    static constexpr size_t IDX_HALF_TRIPLET = 4;
    static constexpr size_t IDX_QUARTER      = 5;
    static constexpr size_t IDX_EIGHTH       = 6;

    static musical_time_t barLengthOf(TimeSignature sig);
    musical_time_t intervalOf(size_t idx) const;
    static tick_timing_t maskOf(size_t idx);
    // ロック取得中に現在の累積 musical_time を計算する
    musical_time_t computeCurrentMusicalTimeLocked() const;

    void timerWorkInner();
    static void timerWork(TimerHandle_t t);

    // 専用タスク: tickQueue から TickInfo を受け取りリスナーへ onTick を配送する
    void dispatchLoop();
    static void dispatchTaskEntry(void *arg);
};

extern TempoController Tempo;
