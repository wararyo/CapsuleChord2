#ifndef _BLEMIDI_H_
#define _BLEMIDI_H_

#include <NimBLEDevice.h>
#include <functional>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define MIDI_SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define MIDI_CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

// BLE MIDIの最大パケットサイズ（MTUに依存、通常20バイト程度）
#define BLE_MIDI_MAX_PACKET_SIZE 20

// 1パケットに入るMIDIメッセージ数（header 1byte + (timestamp 1byte + message 3bytes) * n）
#define BLE_MIDI_MESSAGES_PER_PACKET 4

// バッファフラッシュ間隔（ミリ秒）
#define BLE_MIDI_FLUSH_INTERVAL_MS 15

// FreeRTOS Queueのサイズ（MIDIメッセージ数）
#define BLE_MIDI_QUEUE_SIZE 64

// フラッシュタスクのスタックサイズとプライオリティ
// ボンディング有効時はNimBLEの暗号化処理でスタックを多く消費するため大きめに設定
#define BLE_MIDI_TASK_STACK_SIZE 4096
#define BLE_MIDI_TASK_PRIORITY 1
#define BLE_MIDI_TASK_CORE 0

// MIDIメッセージ構造体（Queue用）
struct MidiMessage {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
};

class BLEMidi : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
public:
    bool isConnected = false;

    // コールバック関数
    std::function<void()> onConnectCallback = nullptr;
    std::function<void()> onDisconnectCallback = nullptr;

    void begin(const std::string& deviceName);
    void end();

    // MIDIメッセージ送信（Queueに追加、タスクで自動送信）
    void sendNoteOn(uint8_t channel, uint8_t noteNo, uint8_t vel);
    void sendNoteOff(uint8_t channel, uint8_t noteNo);
    void sendCC(uint8_t channel, uint8_t ccNo, uint8_t value);

    // 即時フラッシュ（必要な場合）
    void flush();

    // NimBLEServerCallbacks
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;

    // NimBLECharacteristicCallbacks
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;

private:
    NimBLEServer* server = nullptr;
    NimBLECharacteristic* characteristic = nullptr;
    NimBLEAdvertising* advertising = nullptr;
    bool initialized = false;

    // FreeRTOS Queue（FIFO）
    QueueHandle_t messageQueue = nullptr;

    // フラッシュタスク
    TaskHandle_t flushTaskHandle = nullptr;
    volatile bool taskRunning = false;

    void sendPacket(std::vector<MidiMessage>& messages);
    void addMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);

    // タスク関数（static）
    static void flushTask(void* param);
};

extern BLEMidi Midi;

#endif
