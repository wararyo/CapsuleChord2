#pragma once

#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

/**
 * I2Cバスを使用するすべての処理を一つのスレッドで管理するクラス
 * 複数の処理がI2Cバスを取り合うことを防ぐ
 */
class I2CHandler {
public:
    struct TouchData {
        bool isTouched;
        uint16_t x;
        uint16_t y;
    };

    /**
     * I2Cハンドラーを初期化し、専用スレッドを開始する
     */
    void begin();

    /**
     * I2Cハンドラーを終了し、スレッドを停止する
     */
    void terminate();

    /**
     * タッチデータを取得する（スレッドセーフ）
     * @param touchData タッチデータを格納する構造体へのポインタ
     * @return タッチデータが取得できた場合true
     */
    bool getTouchData(TouchData* touchData);

    bool resetTouchData();

    /**
     * I2Cスレッドにシャットダウンを要求する。
     * 電源ボタン長押しと同じシーケンス（LED消灯→電源OFF）が実行される。
     */
    void requestShutdown() { shutdownRequested.store(true); }

private:
    static void i2cTaskLoop(void* parameter);
    void i2cLoop();
    void updateTouchData();

    TaskHandle_t i2cTaskHandle = nullptr;
    SemaphoreHandle_t touchDataMutex = nullptr;
    TouchData currentTouchData = {false, 0, 0};
    bool resetTouchDataRequested = false;
    volatile bool taskRunning = false;  // タスク停止フラグ
    std::atomic<bool> shutdownRequested{false};
};

extern I2CHandler I2C;
