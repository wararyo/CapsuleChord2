#include "BLEMidi.h"
#include <Arduino.h>

BLEMidi Midi;

void BLEMidi::begin(const std::string& deviceName) {
    if (initialized) return;

    Serial.println("BLEMidi: Initializing NimBLE...");

    // Queue作成
    if (messageQueue == nullptr) {
        messageQueue = xQueueCreate(BLE_MIDI_QUEUE_SIZE, sizeof(MidiMessage));
    }

    // NimBLEデバイス初期化
    NimBLEDevice::init(deviceName);

    // セキュリティ設定（Windowsでボンディングを有効にするために使用）
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT); // Just Worksペアリング
    // ボンディング情報の交換キー設定
    // イニシエータ（Central/PC）とレスポンダ（Peripheral/本デバイス）の両方でキーを交換
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    
    Serial.printf("BLEMidi: numBonds = %d\n", NimBLEDevice::getNumBonds());

    // サーバー作成
    server = NimBLEDevice::createServer();
    server->setCallbacks(this);

    // MIDIサービス作成
    NimBLEService* service = server->createService(MIDI_SERVICE_UUID);

    // MIDIキャラクタリスティック作成
    characteristic = service->createCharacteristic(
        MIDI_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE_NR |
        NIMBLE_PROPERTY::NOTIFY
    );

    // サービス開始
    service->start();

    // アドバタイジング設定
    advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(MIDI_SERVICE_UUID);
    advertising->setName(deviceName);

    // アドバタイジング開始
    advertising->start();

    initialized = true;

    // フラッシュタスク開始
    taskRunning = true;
    xTaskCreatePinnedToCore(
        flushTask,
        "BLEMidiFlush",
        BLE_MIDI_TASK_STACK_SIZE,
        this,
        BLE_MIDI_TASK_PRIORITY,
        &flushTaskHandle,
        BLE_MIDI_TASK_CORE
    );

    Serial.println("BLEMidi: BLE MIDI Server started, advertising...");
}

void BLEMidi::end() {
    if (!initialized) return;

    // タスク停止
    taskRunning = false;
    if (flushTaskHandle != nullptr) {
        // タスクが終了するのを待つ
        vTaskDelay(pdMS_TO_TICKS(BLE_MIDI_FLUSH_INTERVAL_MS * 2));
        flushTaskHandle = nullptr;
    }

    // 残りのメッセージをフラッシュ
    flush();

    // アドバタイジング停止
    if (advertising) {
        advertising->stop();
    }

    // NimBLEを停止（メモリは解放しない、再初期化を容易にするため）
    NimBLEDevice::deinit(false);

    initialized = false;
    isConnected = false;

    // Queueをクリア
    if (messageQueue != nullptr) {
        xQueueReset(messageQueue);
    }

    Serial.println("BLEMidi: BLE MIDI stopped");
}

void BLEMidi::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    isConnected = true;
    Serial.printf("BLEMidi: Client connected, address: %s\n",
                  connInfo.getAddress().toString().c_str());

    if (onConnectCallback) {
        onConnectCallback();
    }
}

void BLEMidi::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    isConnected = false;
    Serial.printf("BLEMidi: Client disconnected, reason: %d\n", reason);

    // Queueをクリア
    if (messageQueue != nullptr) {
        xQueueReset(messageQueue);
    }

    if (onDisconnectCallback) {
        onDisconnectCallback();
    }

    // 再アドバタイジング
    if (advertising) {
        advertising->start();
        Serial.println("BLEMidi: Restarted advertising");
    }
}

void BLEMidi::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    // MIDI入力を受信した場合
    Serial.println("BLEMidi: Received MIDI data");
}

void BLEMidi::sendPacket(std::vector<MidiMessage>& messages) {
    if (!isConnected || !characteristic || messages.empty()) return;

    // パケット構築: [header, (timestamp, status, data1, data2), ...]
    std::vector<uint8_t> packet;
    packet.push_back(0x80);  // header byte

    for (const auto& msg : messages) {
        packet.push_back(0x80);  // timestamp
        packet.push_back(msg.status);
        packet.push_back(msg.data1);
        packet.push_back(msg.data2);
    }

    characteristic->setValue(packet.data(), packet.size());
    characteristic->notify();
}

void BLEMidi::addMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    if (!isConnected || messageQueue == nullptr) return;

    MidiMessage msg = {status, data1, data2};

    // Queueに追加（ブロックせずに即座に戻る）
    // Queueがいっぱいの場合はメッセージを破棄
    xQueueSend(messageQueue, &msg, 0);
}

void BLEMidi::sendNoteOn(uint8_t channel, uint8_t noteNo, uint8_t vel) {
    uint8_t status = 0x90 | (channel & 0x0F);
    addMidiMessage(status, noteNo & 0x7F, vel & 0x7F);
}

void BLEMidi::sendNoteOff(uint8_t channel, uint8_t noteNo) {
    uint8_t status = 0x80 | (channel & 0x0F);
    addMidiMessage(status, noteNo & 0x7F, 0);
}

void BLEMidi::sendCC(uint8_t channel, uint8_t ccNo, uint8_t value) {
    uint8_t status = 0xB0 | (channel & 0x0F);
    addMidiMessage(status, ccNo & 0x7F, value & 0x7F);
}

void BLEMidi::flush() {
    if (messageQueue == nullptr || !isConnected) return;

    // Queueから全メッセージを取り出して送信
    std::vector<MidiMessage> messages;
    MidiMessage msg;

    while (xQueueReceive(messageQueue, &msg, 0) == pdTRUE) {
        messages.push_back(msg);

        // パケットサイズ上限に達したら送信
        if (messages.size() >= BLE_MIDI_MESSAGES_PER_PACKET) {
            sendPacket(messages);
            messages.clear();
        }
    }

    // 残りを送信
    if (!messages.empty()) {
        sendPacket(messages);
    }
}

// フラッシュタスク（Core1で実行）
void BLEMidi::flushTask(void* param) {
    BLEMidi* self = static_cast<BLEMidi*>(param);
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(BLE_MIDI_FLUSH_INTERVAL_MS);

    while (self->taskRunning) {
        if (self->isConnected && self->messageQueue != nullptr) {
            // Queueからメッセージを取り出してパケットサイズ分だけ送信
            std::vector<MidiMessage> messages;
            MidiMessage msg;

            // 1パケット分だけ取り出す
            while (messages.size() < BLE_MIDI_MESSAGES_PER_PACKET &&
                   xQueueReceive(self->messageQueue, &msg, 0) == pdTRUE) {
                messages.push_back(msg);
            }

            if (!messages.empty()) {
                self->sendPacket(messages);
            }
        }

        // 正確な間隔で実行
        vTaskDelayUntil(&lastWakeTime, interval);
    }

    // タスク終了
    vTaskDelete(nullptr);
}
