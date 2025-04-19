#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <list>
#include <cstdint>
#include "Foundation/MusicalTime.h"

// テンポの管理と通知を行うクラス
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
    // テンポの変更を通知するためのインターフェース
    // 各種コールバック内でTempo.start()やTempo.stop()を呼び出さない！(デッドロックが発生するため)
    class TempoCallbacks
    {
    public:
        virtual void onPlayingStateChanged(bool isPlaying) = 0;
        virtual void onTempoChanged(tempo_t tempo) = 0;
        virtual void onTick(tick_timing_t timing, musical_time_t time) = 0;
        // Tickを通知するタイミング
        // tick_beat_tのビットフラグで指定する
        virtual tick_timing_t getTimingMask()
        {
            return TICK_TIMING_BAR; // TODO: 0が初期値でいいよね多分
        }
    };

    // テンポを取得する
    tempo_t getTempo() const
    {
        return tempo;
    }

    // カウント開始からの積算音楽時間を取得する
    musical_time_t getMusicalTime() const
    {
        return musicalTime;
    }

    // テンポを変更する
    void setTempo(tempo_t tempo)
    {
        this->tempo = tempo;
        for (TempoCallbacks *listener : listeners)
        {
            listener->onTempoChanged(tempo);
        }
    }

    bool getPlaying() const
    {
        return isPlaying;
    }

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

private:
    bool isPlaying = false;
    portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    tempo_t tempo = 110;
    // beat = BEAT_4_4;
    std::list<TempoCallbacks *> listeners;
    TimerHandle_t timer = nullptr;
    // 直前の小節頭からの経過時間(ミリ秒)
    uint32_t elapsedTime = 0;
    // elapsedTimeがこれら以上である場合、Tickを通知する
    float nextTimeToTickBar = 0;
    float nextTimeToTickFullBeat = 0;
    float nextTimeToTickFullBeatTriplet = 0;
    float nextTimeToTickHalfBeat = 0;
    float nextTimeToTickHalfBeatTriplet = 0;
    float nextTimeToTickQuarterBeat = 0;
    float nextTimeToTickEighthBeat = 0;
    uint32_t nearestNextTimeToTick = 0; // 高速化のために、直近でトリガーされる時間は整数で用意しておく
    float intervalBar = 0.0f;
    float intervalFullBeat = 0.0f;
    float intervalFullBeatTriplet = 0.0f;
    float intervalHalfBeat = 0.0f;
    float intervalHalfBeatTriplet = 0.0f;
    float intervalQuarterBeat = 0.0f;
    float intervalEighthBeat = 0.0f;

    // カウント開始からの積算音楽時間
    musical_time_t musicalTime = 0;

    void timerWorkInner();
    static void timerWork(TimerHandle_t t);
};

extern TempoController Tempo;
