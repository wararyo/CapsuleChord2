#include "Keypad.h"
#include <M5Unified.h>

#define EXT_I2C_PORT 0

#define PORTA_SCL  1
#define PORTA_SDA  2

void CapsuleChordKeypad::begin() {
    M5.Ex_I2C.begin(EXT_I2C_PORT, PORTA_SDA, PORTA_SCL);
    // キーパッド側のイベントキューをクリア
    while (true) {
        Wire.requestFrom(KEYPAD_I2C_ADDR,1);
        if (!Wire.available()) break;
        if (Wire.read() == 0) break;
    }

    // LEDベースレイヤーを登録
    auto baseLayer = std::make_shared<LedLayer>("Base Layer");
    baseLayer->fillLeds(LED_DIM);
    pushLedLayer(baseLayer);
}

void CapsuleChordKeypad::update() {
    Wire.beginTransmission(KEYPAD_I2C_ADDR);
    Wire.write(CMD_GET_KEY_EVENT);
    Wire.endTransmission();

    Wire.requestFrom(KEYPAD_I2C_ADDR,1);//TODO:4とか試してみる
    while (Wire.available()) {
        int val = Wire.read();
        if(val != 0) {
            // KeyEventオブジェクトを作成
            KeyEvent event(static_cast<char>(val));
            
            // Update keys
            int keyCode = event.getKeyCode();
            if (keys.find(keyCode) != keys.end())
            {
                if(event.isPressed()) keys[keyCode].press();
                else keys[keyCode].release();
            }
            else
            {
                keys[keyCode] = Key();
                if(event.isPressed()) keys[keyCode].press();
                else keys[keyCode].release();
            }
            
            // Process the event through listener stack
            if (processKeyEvent(event)) {
                // Event was consumed by a listener
                continue;
            }
        }
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
    
    // Send command through I2C
    Wire.beginTransmission(KEYPAD_I2C_ADDR);
    Wire.write(CMD_SET_LED);    // LED command
    Wire.write(keyCode);        // Key code
    Wire.write(brightness);     // Brightness level
    Wire.endTransmission();
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
    Serial.printf("LED Layer pushed: %s (stack size: %d)\n", 
                  layer->getName().c_str(), _ledLayers.size());
}

void CapsuleChordKeypad::removeLedLayer(std::shared_ptr<LedLayer> layer) {
    if (!layer) return;
    
    for (auto it = _ledLayers.begin(); it != _ledLayers.end(); ++it) {
        if (*it == layer) {
            _ledLayers.erase(it);
            _needsLedUpdate = true;
            Serial.printf("LED Layer removed: %s (stack size: %d)\n", 
                          layer->getName().c_str(), _ledLayers.size());
            break;
        }
    }
}

void CapsuleChordKeypad::markLedNeedsUpdate() {
    _needsLedUpdate = true;
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
    
    Serial.printf("LEDs updated from layer: %s\n", topLayer->getName().c_str());
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