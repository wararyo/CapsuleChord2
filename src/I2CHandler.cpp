#include "I2CHandler.h"
#include "Keypad.h"
#include <M5Unified.h>

void I2CHandler::begin() {
    // ミューテックスを作成
    touchDataMutex = xSemaphoreCreateMutex();
    if (touchDataMutex == nullptr) {
        Serial.println("Failed to create touch data mutex");
        return;
    }

    // タッチデータを初期化
    currentTouchData = {false, 0, 0};

    // Core1でI2Cタスクを開始
    xTaskCreatePinnedToCore(
        i2cTaskLoop,
        "i2cHandler",
        4096,
        this,
        2,  // 高い優先度を設定
        &i2cTaskHandle,
        1   // Core1で実行
    );

    Serial.println("I2C Handler started on Core1");
}

void I2CHandler::terminate() {
    // タスクを停止
    if (i2cTaskHandle != nullptr) {
        vTaskDelete(i2cTaskHandle);
        i2cTaskHandle = nullptr;
    }

    // ミューテックスを削除
    if (touchDataMutex != nullptr) {
        vSemaphoreDelete(touchDataMutex);
        touchDataMutex = nullptr;
    }

    Serial.println("I2C Handler terminated");
}

bool I2CHandler::getTouchData(TouchData* touchData) {
    if (touchData == nullptr || touchDataMutex == nullptr) {
        return false;
    }

    // ミューテックスを取得してタッチデータを安全にコピー
    if (xSemaphoreTake(touchDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        *touchData = currentTouchData;
        xSemaphoreGive(touchDataMutex);
        return true;
    }

    return false;
}

bool I2CHandler::resetTouchData() {
    if (touchDataMutex == nullptr) return false;

    // ミューテックスを取得してタッチデータをリセット
    if (xSemaphoreTake(touchDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        resetTouchDataRequested = true;
        xSemaphoreGive(touchDataMutex);
        return true;
    }

    return false;
}

void I2CHandler::i2cTaskLoop(void* parameter) {
    I2CHandler* handler = static_cast<I2CHandler*>(parameter);
    handler->i2cLoop();
}

void I2CHandler::i2cLoop() {
    const TickType_t xDelay = pdMS_TO_TICKS(5);
    
    Serial.println("I2C thread loop started");
    
    static unsigned long lastKeypadUpdate = 0;
    const unsigned long keypadInterval = 15;
    
    while (true) {
        unsigned long startTime = millis();

        // M5の状態を更新（バッテリー、IMUなど）
        M5.update();

        // キーパッドの状態を更新（15ms間隔で実行）
        if (startTime - lastKeypadUpdate >= keypadInterval) {
            Keypad.update();
            lastKeypadUpdate = startTime;
        }

        // タッチデータを更新
        updateTouchData();

        unsigned long endTime = millis();
        unsigned long elapsedTime = endTime - startTime;

        // デバッグ用：処理時間が長い場合は警告（最初の数回は除く）
        static int loopCount = 0;
        loopCount++;
        if (elapsedTime > 10 && loopCount > 10) {
            Serial.printf("I2C loop took %lu ms (warning threshold: 10ms)\n", elapsedTime);
        }

        vTaskDelay(xDelay);
    }
}

void I2CHandler::updateTouchData() {
    if (touchDataMutex == nullptr) return;

    uint16_t touchX, touchY;
    bool isTouched = M5.Display.getTouch(&touchX, &touchY);

    // ミューテックスを取得してタッチデータを更新（ブロッキングを避けるため短時間でタイムアウト）
    if (xSemaphoreTake(touchDataMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
        if (resetTouchDataRequested) {
            currentTouchData.isTouched = false;
            resetTouchDataRequested = false;
        }
        if (isTouched) {
            currentTouchData.isTouched = true;
            currentTouchData.x = touchX;
            currentTouchData.y = touchY;
        }
        xSemaphoreGive(touchDataMutex);
    } else {
        // ミューテックスの取得に失敗した場合のログ（頻繁すぎないよう制限）
        static unsigned long lastWarning = 0;
        unsigned long now = millis();
        if (now - lastWarning > 1000) { // 1秒に1回まで
            Serial.println("Warning: Failed to acquire touch data mutex in I2C thread");
            lastWarning = now;
        }
    }
}

I2CHandler I2C;
