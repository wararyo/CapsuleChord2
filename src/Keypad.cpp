#include "Keypad.h"
#include <M5Unified.h>

#define EXT_I2C_PORT 0

#define PORTA_SCL  1
#define PORTA_SDA  2

void CapsuleChordKeypad::begin() {
    M5.Ex_I2C.begin(EXT_I2C_PORT, PORTA_SDA, PORTA_SCL);
}

void CapsuleChordKeypad::update() {
    Wire.requestFrom(KEYPAD_I2C_ADDR,1);//TODO:4とか試してみる
    while(Wire.available()) {
        int val = Wire.read();
        if(val != 0) {
            // KeyEventオブジェクトを作成
            KeyEvent event(static_cast<char>(val));
            
            // Update keys
            int keyCode = event.getKeyCode();
            if(keys.find(keyCode) != keys.end()) {
                if(event.isPressed()) keys[keyCode].press();
                else keys[keyCode].release();
            }
            
            // Process the event through listener stack
            if (processKeyEvent(event)) {
                // Event was consumed by a listener
                continue;
            }
            
            // If not consumed, add to queue for chord system
            _events.push(event);
        }
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

bool CapsuleChordKeypad::hasEvent() {
    return !_events.empty();
}

KeyEvent CapsuleChordKeypad::getEvent() {
    KeyEvent event = _events.front();
    _events.pop();
    return event;
}

void CapsuleChordKeypad::disposeEvents() {
    std::queue<KeyEvent> empty;
    std::swap(_events, empty);
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