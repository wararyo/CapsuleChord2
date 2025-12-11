#include "BLEMidi.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char* LOG_TAG = "BLEMidi";

BLEMidi Midi;

// MIDI Service UUID: 03b80e5a-ede8-4b33-a751-6ce34ec4c700
static const ble_uuid128_t midiServiceUuid = BLE_UUID128_INIT(
    0x00, 0xc7, 0xc4, 0x4e, 0xe3, 0x6c, 0x51, 0xa7,
    0x33, 0x4b, 0xe8, 0xed, 0x5a, 0x0e, 0xb8, 0x03
);

// MIDI Characteristic UUID: 7772e5db-3868-4112-a1a9-f2669d106bf3
static const ble_uuid128_t midiCharUuid = BLE_UUID128_INIT(
    0xf3, 0x6b, 0x10, 0x9d, 0x66, 0xf2, 0xa9, 0xa1,
    0x12, 0x41, 0x68, 0x38, 0xdb, 0xe5, 0x72, 0x77
);

// Global pointer for static callbacks
static BLEMidi* gBLEMidiInstance = nullptr;

// GATT service definition
static const struct ble_gatt_svc_def gattServices[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &midiServiceUuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &midiCharUuid.u,
                .access_cb = BLEMidi::gattAccessCallback,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = nullptr,  // Will be set during registration
            },
            { 0 }  // End of characteristics
        },
    },
    { 0 }  // End of services
};

void BLEMidi::begin(const std::string& name) {
    // Mutex作成（初回のみ）
    if (lifecycleMutex == nullptr) {
        lifecycleMutex = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(lifecycleMutex, portMAX_DELAY);

    if (initialized) {
        xSemaphoreGive(lifecycleMutex);
        return;
    }

    ESP_LOGI(LOG_TAG, "Initializing NimBLE...");

    deviceName = name;
    gBLEMidiInstance = this;

    // Queue作成
    if (messageQueue == nullptr) {
        messageQueue = xQueueCreate(BLE_MIDI_QUEUE_SIZE, sizeof(MidiMessage));
    }

    // Initialize NVS (required for NimBLE bonding)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize NimBLE
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to init nimble port: %d", ret);
        xSemaphoreGive(lifecycleMutex);
        return;
    }

    // Configure the host
    ble_hs_cfg.reset_cb = onReset;
    ble_hs_cfg.sync_cb = onSync;
    ble_hs_cfg.gatts_register_cb = nullptr;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // Security settings for bonding
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    // Initialize GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Register custom GATT services
    int rc = ble_gatts_count_cfg(gattServices);
    if (rc != 0) {
        ESP_LOGE(LOG_TAG, "Failed to count GATT services: %d", rc);
        xSemaphoreGive(lifecycleMutex);
        return;
    }

    rc = ble_gatts_add_svcs(gattServices);
    if (rc != 0) {
        ESP_LOGE(LOG_TAG, "Failed to add GATT services: %d", rc);
        xSemaphoreGive(lifecycleMutex);
        return;
    }

    // Set device name
    rc = ble_svc_gap_device_name_set(deviceName.c_str());
    if (rc != 0) {
        ESP_LOGE(LOG_TAG, "Failed to set device name: %d", rc);
    }

    // Start NimBLE host task
    nimble_port_freertos_init(nimbleHostTask);

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

    xSemaphoreGive(lifecycleMutex);

    ESP_LOGI(LOG_TAG, "BLE MIDI Server started");
}

void BLEMidi::end() {
    if (lifecycleMutex == nullptr) return;

    xSemaphoreTake(lifecycleMutex, portMAX_DELAY);

    if (!initialized) {
        xSemaphoreGive(lifecycleMutex);
        return;
    }

    ESP_LOGI(LOG_TAG, "Stopping BLE MIDI...");

    // タスク停止フラグを設定
    taskRunning = false;

    // タスクが自己削除するのを待つ（最大200ms）
    if (flushTaskHandle != nullptr) {
        ESP_LOGI(LOG_TAG, "Waiting for flush task to stop...");
        for (int i = 0; i < 20 && eTaskGetState(flushTaskHandle) != eDeleted; i++) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (eTaskGetState(flushTaskHandle) != eDeleted) {
            ESP_LOGW(LOG_TAG, "Flush task did not stop gracefully, forcing delete");
            vTaskDelete(flushTaskHandle);
        }
        flushTaskHandle = nullptr;
        ESP_LOGI(LOG_TAG, "Flush task stopped");
    }

    // Stop advertising if connected
    if (connHandle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(connHandle, BLE_ERR_REM_USER_CONN_TERM);
    }

    // Deinitialize NimBLE
    int ret = nimble_port_stop();
    if (ret == 0) {
        nimble_port_deinit();
    }

    initialized = false;
    isConnected = false;
    connHandle = BLE_HS_CONN_HANDLE_NONE;
    gBLEMidiInstance = nullptr;

    // Queueを削除
    if (messageQueue != nullptr) {
        vQueueDelete(messageQueue);
        messageQueue = nullptr;
        ESP_LOGI(LOG_TAG, "Message queue deleted");
    }

    xSemaphoreGive(lifecycleMutex);

    ESP_LOGI(LOG_TAG, "BLE MIDI stopped");
}

void BLEMidi::nimbleHostTask(void* param) {
    ESP_LOGI(LOG_TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void BLEMidi::onSync() {
    ESP_LOGI(LOG_TAG, "NimBLE host synced");

    // Make sure we have proper identity address set
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(LOG_TAG, "Failed to ensure address: %d", rc);
        return;
    }

    // Start advertising
    if (gBLEMidiInstance) {
        gBLEMidiInstance->startAdvertising();
    }
}

void BLEMidi::onReset(int reason) {
    ESP_LOGW(LOG_TAG, "NimBLE host reset, reason: %d", reason);
}

void BLEMidi::startAdvertising() {
    struct ble_gap_adv_params advParams;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));

    // Advertise flags
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Include complete 128-bit service UUID
    fields.uuids128 = &midiServiceUuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    // Device name
    fields.name = (uint8_t*)deviceName.c_str();
    fields.name_len = deviceName.length();
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(LOG_TAG, "Failed to set adv fields: %d", rc);
        return;
    }

    // Set advertising parameters
    memset(&advParams, 0, sizeof(advParams));
    advParams.conn_mode = BLE_GAP_CONN_MODE_UND;
    advParams.disc_mode = BLE_GAP_DISC_MODE_GEN;
    advParams.itvl_min = 0x20;  // 20ms
    advParams.itvl_max = 0x40;  // 40ms

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &advParams, gapEventHandler, this);
    if (rc != 0) {
        ESP_LOGE(LOG_TAG, "Failed to start advertising: %d", rc);
        return;
    }

    ESP_LOGI(LOG_TAG, "Advertising started");
}

int BLEMidi::gapEventHandler(struct ble_gap_event *event, void *arg) {
    BLEMidi* self = static_cast<BLEMidi*>(arg);
    if (!self) self = gBLEMidiInstance;
    if (!self) return 0;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(LOG_TAG, "Client connected, conn_handle=%d", event->connect.conn_handle);
                self->connHandle = event->connect.conn_handle;
                self->isConnected = true;

                if (self->onConnectCallback) {
                    self->onConnectCallback();
                }
            } else {
                ESP_LOGW(LOG_TAG, "Connection failed, status=%d", event->connect.status);
                self->startAdvertising();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(LOG_TAG, "Client disconnected, reason=%d", event->disconnect.reason);
            self->connHandle = BLE_HS_CONN_HANDLE_NONE;
            self->isConnected = false;

            // Clear queue
            if (self->messageQueue != nullptr) {
                xQueueReset(self->messageQueue);
            }

            if (self->onDisconnectCallback) {
                self->onDisconnectCallback();
            }

            // Restart advertising
            self->startAdvertising();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(LOG_TAG, "Advertising complete");
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(LOG_TAG, "Subscribe event, attr_handle=%d", event->subscribe.attr_handle);
            // Store the attribute handle for notifications
            if (event->subscribe.cur_notify) {
                self->midiCharAttrHandle = event->subscribe.attr_handle;
            }
            break;

        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(LOG_TAG, "MTU update, conn_handle=%d, mtu=%d",
                     event->mtu.conn_handle, event->mtu.value);
            break;

        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGI(LOG_TAG, "Encryption change, status=%d", event->enc_change.status);
            break;

        default:
            break;
    }

    return 0;
}

int BLEMidi::gattAccessCallback(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ESP_LOGI(LOG_TAG, "GATT read");
            break;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            ESP_LOGI(LOG_TAG, "GATT write (MIDI data received)");
            break;

        default:
            break;
    }

    return 0;
}

void BLEMidi::sendPacket(std::vector<MidiMessage>& messages) {
    if (!isConnected || messages.empty() || connHandle == BLE_HS_CONN_HANDLE_NONE) return;

    // パケット構築: [header, (timestamp, status, data1, data2), ...]
    std::vector<uint8_t> packet;
    packet.push_back(0x80);  // header byte

    for (const auto& msg : messages) {
        packet.push_back(0x80);  // timestamp
        packet.push_back(msg.status);
        packet.push_back(msg.data1);
        packet.push_back(msg.data2);
    }

    // Send notification
    struct os_mbuf *om = ble_hs_mbuf_from_flat(packet.data(), packet.size());
    if (om) {
        int rc = ble_gatts_notify_custom(connHandle, midiCharAttrHandle, om);
        if (rc != 0) {
            ESP_LOGW(LOG_TAG, "Failed to send notification: %d", rc);
        }
    }
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

// フラッシュタスク（Core0で実行）
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
