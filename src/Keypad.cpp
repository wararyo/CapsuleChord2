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
            // Update keys
            int state = (val & 0b10000000) >> 7;
            int key   = val & 0b01111111;
            if(keys.find(key) != keys.end()) {
                if(state == KEY_STATE_PRESSED) keys[key].press();
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