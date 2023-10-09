#include "Keypad.h"
#include <M5Unified.h>

void CapsuleChordKeypad::begin() {
    Wire1.begin();
}

void CapsuleChordKeypad::update() {
    Serial.println("Keypad::update 1");
    Wire1.requestFrom(KEYPAD_I2C_ADDR,1);//TODO:4とか試してみる
    Serial.println("Keypad::update 2");
    while(Wire1.available()) {
        int val = Wire1.read();
        Serial.println("Keypad::update 3");
        if(val != 0) {
            // Update keys
            int state = val & 0b10000000;
            int key   = val & 0b01111111;
            if(keys.find(key) != keys.end()) {
                if(state == Key_State_Pressed) keys[key].press();
                else keys[key].release();
            }
            _events.push((char)val);
        }
    }
}

bool CapsuleChordKeypad::hasEvent() {
    return !_events.empty();
}

char CapsuleChordKeypad::getEvent() {
    char event = _events.front();
    _events.pop();
    return event;
}

void CapsuleChordKeypad::disposeEvents() {
    std::queue<char> empty;
    std::swap( _events, empty );
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