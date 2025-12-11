#include "AppSoundTest.h"
#include <esp_timer.h>

// ESP-IDF millis replacement
static inline uint32_t esp_millis() {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void AppSoundTest::onCreate()
{
}

void AppSoundTest::onActivate()
{
}

void AppSoundTest::onDeactivate()
{
}

void AppSoundTest::onShowGui(lv_obj_t *container)
{
    // アプリタイトル
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 4);

    // ボタン
    button = lv_btn_create(container);
    lv_obj_set_size(button, 136, 80);
    lv_obj_center(button);
    lv_obj_add_event_cb(button, [](lv_event_t *e) {
        // タスクを開始
        auto *self = (AppSoundTest *)lv_event_get_user_data(e);
        xTaskCreate(startPlayer, "soundtest", 4096, self, 1, &self->taskHandle);
    }, LV_EVENT_CLICKED, (void *)this);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "GO!");
    lv_obj_center(label);
}

void AppSoundTest::onHideGui()
{
    lv_obj_del(titleLabel);
    lv_obj_del(button);
}

void AppSoundTest::onDestroy()
{
}

// 曲を再生するタスク
void AppSoundTest::playerLoop()
{
    TickType_t delayTime = pdMS_TO_TICKS(1);
    uint32_t startTime = esp_millis();
    const MidiMessage *nextMessage = song; // 次に処理するべきMIDIメッセージ
    uint32_t nextGoal = nextMessage->time;
    bool hasReachedEndOfSong = false;  // 多重ループを抜けるために使用
    while (true) {
        uint32_t time = esp_millis() - startTime;
        while (time >= nextGoal) {
            if ((nextMessage->status & 0xF0) == 0x90)
            {
                Output.Internal.NoteOn(nextMessage->data1, nextMessage->data2, nextMessage->status & 0x0F);
            }
            else if ((nextMessage->status & 0xF0) == 0x80)
            {
                Output.Internal.NoteOff(nextMessage->data1, nextMessage->data2, nextMessage->status & 0x0F);
            }
            else if ((nextMessage->status & 0xF0) == 0xE0)
            {
                uint_fast16_t rawValue = (nextMessage->data2 & 0b01111111) << 7 | (nextMessage->data1 & 0b01111111);
                int16_t value = rawValue - 8192;
                Output.Internal.PitchBend(value, nextMessage->status & 0x0F);
            }
            if (nextMessage->status == 0xFF && nextMessage->data1 == 0x2F && nextMessage->data2 == 0x00)
            {
                // 0xFF, 0x2F, 0x00 は曲の終わりを意味する
                hasReachedEndOfSong = true;
                break;
            }
            nextMessage++;
            nextGoal = nextMessage->time;
        }

        if (hasReachedEndOfSong) break;
        vTaskDelay(delayTime);
    }
    vTaskDelete(taskHandle);
}

void AppSoundTest::startPlayer(void* _this)
{
    ((AppSoundTest*)_this)->playerLoop();
}
