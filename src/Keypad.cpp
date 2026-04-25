#include "Keypad.h"
#include <M5Unified.h>
#include <esp_log.h>

static const char* LOG_TAG = "Keypad";

#define EXT_I2C_PORT I2C_NUM_0

#define PORTA_SCL  1
#define PORTA_SDA  2

void CapsuleChordKeypad::begin() {
    if (_initialized) return;  // 多重初期化を防止

    M5.Ex_I2C.begin(EXT_I2C_PORT, PORTA_SDA, PORTA_SCL);

    // Step 1: 旧FW互換のbareリードでイベントキューをドレインする。
    // 新FWでpointer未設定のbareリードが非ゼロを返し続ける場合に備え、
    // 反復回数に上限を設ける。
    for (int i = 0; i < 32; i++) {
        uint8_t data = 0;
        if (!M5.Ex_I2C.start(KEYPAD_I2C_ADDR, true, 400000)) break;
        if (!M5.Ex_I2C.read(&data, 1)) {
            M5.Ex_I2C.stop();
            break;
        }
        M5.Ex_I2C.stop();
        if (data == 0) break;
    }

    // Step 2: キューが空になったうえでFWバージョンを問い合わせ、
    // プロトコル種別を判定する。
    _protocol = detectProtocol();

    // Step 3: 新FWの場合はREG_KEY_EVENT経由でもう一度ドレインしておく
    // (step1のbareリードが新FWで効かない可能性があるため)。
    if (_protocol == KeypadProtocol::V3) {
        for (int i = 0; i < 32; i++) {
            if (readKeyEventV3() == 0) break;
        }
    }

    // LEDベースレイヤーを登録
    auto baseLayer = std::make_shared<LedLayer>("Base Layer");
    baseLayer->fillLeds(LED_DIM);
    pushLedLayer(baseLayer);

    _initialized = true;
}

KeypadProtocol CapsuleChordKeypad::detectProtocol() {
    uint8_t regAddr = REG_FW_VERSION;
    uint8_t version[2] = {0, 0};

    if (!M5.Ex_I2C.start(KEYPAD_I2C_ADDR, false, 400000)) {
        ESP_LOGW(LOG_TAG, "Version probe write-start failed, falling back to legacy");
        return KeypadProtocol::Legacy;
    }
    bool writeOk = M5.Ex_I2C.write(&regAddr, 1);
    M5.Ex_I2C.stop();
    if (!writeOk) {
        ESP_LOGW(LOG_TAG, "Version probe write failed, falling back to legacy");
        return KeypadProtocol::Legacy;
    }

    if (!M5.Ex_I2C.start(KEYPAD_I2C_ADDR, true, 400000)) {
        ESP_LOGW(LOG_TAG, "Version probe read-start failed, falling back to legacy");
        return KeypadProtocol::Legacy;
    }
    bool readOk = M5.Ex_I2C.read(version, 2);
    M5.Ex_I2C.stop();
    if (!readOk) {
        ESP_LOGW(LOG_TAG, "Version probe read failed, falling back to legacy");
        return KeypadProtocol::Legacy;
    }

    if (version[0] >= 3) {
        ESP_LOGI(LOG_TAG, "Keypad firmware v%u.%u detected (new protocol)",
                 version[0], version[1]);
        return KeypadProtocol::V3;
    }

    ESP_LOGI(LOG_TAG, "Legacy keypad firmware detected (version bytes: 0x%02X 0x%02X)",
             version[0], version[1]);
    return KeypadProtocol::Legacy;
}

uint8_t CapsuleChordKeypad::readKeyEventLegacy() {
    uint8_t cmd = CMD_GET_KEY_EVENT;
    uint8_t val = 0;
    if (M5.Ex_I2C.start(KEYPAD_I2C_ADDR, false, 400000)) {
        M5.Ex_I2C.write(&cmd, 1);
        M5.Ex_I2C.stop();
    }
    if (M5.Ex_I2C.start(KEYPAD_I2C_ADDR, true, 400000)) {
        M5.Ex_I2C.read(&val, 1);
        M5.Ex_I2C.stop();
    }
    return val;
}

uint8_t CapsuleChordKeypad::readKeyEventV3() {
    uint8_t regAddr = REG_KEY_EVENT;
    uint8_t val = 0;
    if (M5.Ex_I2C.start(KEYPAD_I2C_ADDR, false, 400000)) {
        M5.Ex_I2C.write(&regAddr, 1);
        M5.Ex_I2C.stop();
    }
    if (M5.Ex_I2C.start(KEYPAD_I2C_ADDR, true, 400000)) {
        M5.Ex_I2C.read(&val, 1);
        M5.Ex_I2C.stop();
    }
    return val;
}

void CapsuleChordKeypad::update() {
    uint8_t val = (_protocol == KeypadProtocol::V3)
        ? readKeyEventV3()
        : readKeyEventLegacy();

    if (val != 0) {
        KeyEvent event(static_cast<char>(val));

        int keyCode = event.getKeyCode();
        if (keys.find(keyCode) == keys.end()) {
            keys[keyCode] = Key();
        }
        if (event.isPressed()) keys[keyCode].press();
        else keys[keyCode].release();

        processKeyEvent(event);
    }

    // Update LEDs if needed
    if (_needsLedUpdate) {
        updateLeds();
        _needsLedUpdate = false;
    }
}

bool CapsuleChordKeypad::processKeyEvent(const KeyEvent& event) {
    uint8_t keyCode = event.getKeyCode();
    
    // Iterate through listeners from top of stack
    for (auto it = _listeners.rbegin(); it != _listeners.rend(); ++it) {
        auto listener = *it;
        
        bool consumed = false;
        
        // Call appropriate handler based on state
        if (event.isPressed()) {
            consumed = listener->onKeyPressed(keyCode);
        } else {
            consumed = listener->onKeyReleased(keyCode);
        }
        
        // If event was consumed, stop propagation
        if (consumed) {
            return true;
        }
    }
    
    // Event wasn't consumed by any listener
    return false;
}

void CapsuleChordKeypad::setLedBrightness(uint8_t keyCode, uint8_t brightness) {
    // Ensure brightness is within valid range (0-3)
    if (brightness > LED_OFF) {
        brightness = LED_OFF;
    }

    if (_protocol == KeypadProtocol::V3) {
        writeLedBrightnessV3(keyCode, brightness);
    } else {
        writeLedBrightnessLegacy(keyCode, brightness);
    }
}

void CapsuleChordKeypad::writeLedBrightnessLegacy(uint8_t keyCode, uint8_t brightness) {
    uint8_t data[3] = {CMD_SET_LED, keyCode, brightness};
    if (M5.Ex_I2C.start(KEYPAD_I2C_ADDR, false, 400000)) {
        M5.Ex_I2C.write(data, 3);
        M5.Ex_I2C.stop();
    }
}

void CapsuleChordKeypad::writeLedBrightnessV3(uint8_t keyCode, uint8_t brightness) {
    // REG_LED_BRIGHT_BASE (0x70) + sparse keycode, 1B data.
    // keyCode が REG_GLOBAL_BRIGHTNESS (0xC8) 以降の領域に被ると別レジスタを誤書きするため、ここで弾く。
    if (keyCode >= (REG_GLOBAL_BRIGHTNESS - REG_LED_BRIGHT_BASE)) {
        ESP_LOGW(LOG_TAG, "keyCode 0x%02X out of V3 LED register range, skipped", keyCode);
        return;
    }
    uint8_t data[2] = {static_cast<uint8_t>(REG_LED_BRIGHT_BASE + keyCode), brightness};
    if (M5.Ex_I2C.start(KEYPAD_I2C_ADDR, false, 400000)) {
        M5.Ex_I2C.write(data, 2);
        M5.Ex_I2C.stop();
    }
}

void CapsuleChordKeypad::addKeyEventListener(std::shared_ptr<KeyEventListener> listener) {
    _listeners.push_back(listener);
}

void CapsuleChordKeypad::removeKeyEventListener(std::shared_ptr<KeyEventListener> listener) {
    for (auto it = _listeners.begin(); it != _listeners.end(); ) {
        if (*it == listener) {
            it = _listeners.erase(it);
            return;
        } else {
            ++it;
        }
    }
}

// LED Layer Management
void CapsuleChordKeypad::pushLedLayer(std::shared_ptr<LedLayer> layer) {
    if (!layer) return;

    _ledLayers.push_back(layer);
    _needsLedUpdate = true;
    ESP_LOGD(LOG_TAG, "LED Layer pushed: %s (stack size: %d)",
             layer->getName().c_str(), _ledLayers.size());
}

void CapsuleChordKeypad::removeLedLayer(std::shared_ptr<LedLayer> layer) {
    if (!layer) return;

    for (auto it = _ledLayers.begin(); it != _ledLayers.end(); ++it) {
        if (*it == layer) {
            _ledLayers.erase(it);
            _needsLedUpdate = true;
            ESP_LOGD(LOG_TAG, "LED Layer removed: %s (stack size: %d)",
                     layer->getName().c_str(), _ledLayers.size());
            break;
        }
    }
}

void CapsuleChordKeypad::markLedNeedsUpdate() {
    _needsLedUpdate = true;
}

void CapsuleChordKeypad::turnOffAllLeds() {
    if (_protocol != KeypadProtocol::V3) return;
    uint8_t data[2] = {REG_GLOBAL_BRIGHTNESS, 0x00};
    if (M5.Ex_I2C.start(KEYPAD_I2C_ADDR, false, 400000)) {
        M5.Ex_I2C.write(data, 2);
        M5.Ex_I2C.stop();
    }
}

void CapsuleChordKeypad::updateLeds() {
    if (_ledLayers.empty()) return;
    
    // Use the topmost layer's LED states
    auto topLayer = _ledLayers.back();
    const auto& ledStates = topLayer->getAllLeds();
    
    // Apply LED states for all known keys
    static const uint8_t allKeys[] = {
        KEY_LEFT_1, KEY_LEFT_2, KEY_LEFT_3, KEY_LEFT_4, KEY_LEFT_5,
        KEY_LEFT_6, KEY_LEFT_7, KEY_LEFT_8, KEY_LEFT_9,
        KEY_RIGHT_1, KEY_RIGHT_2, KEY_RIGHT_3, KEY_RIGHT_4, KEY_RIGHT_5,
        KEY_RIGHT_6, KEY_RIGHT_7, KEY_RIGHT_8, KEY_RIGHT_9,
        KEY_L, KEY_R, KEY_LT, KEY_RT
    };
    
    for (uint8_t keyCode : allKeys) {
        setLedBrightness(keyCode, ledStates[keyCode]);
    }
    
    // Serial.printf("LEDs updated from layer: %s\n", topLayer->getName().c_str());
}

std::shared_ptr<LedLayer> CapsuleChordKeypad::getTopLedLayer() const {
    if (_ledLayers.empty()) {
        return nullptr;
    }
    return _ledLayers.back();
}

void CapsuleChordKeypad::Key::press() {
    mIsPressed = true;
}

void CapsuleChordKeypad::Key::release() {
    mIsPressed = false;
}

bool CapsuleChordKeypad::Key::isPressed() {
    return mIsPressed;
}

CapsuleChordKeypad Keypad;