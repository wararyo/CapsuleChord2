#ifndef _KEYPAD_H_
#define _KEYPAD_H_

#include <Wire.h>
#undef min
#include <queue>
#include <map>

#define KEYPAD_I2C_ADDR 0x09

/*** TWI Structure ***/
// State(1bit) | Button(7bit)
// State: 0 is pressed, 1 is released
// Button: see the following

/*** Buttons ***/
// Group(3bit) | Button(4bit)

#define KEY_STATE_PRESSED 1
#define KEY_STATE_RELEASED 0

// Left keys (Group 0)
#define KEY_LEFT_1 0x01
#define KEY_LEFT_2 0x02
#define KEY_LEFT_3 0x03
#define KEY_LEFT_4 0x04
#define KEY_LEFT_5 0x05
#define KEY_LEFT_6 0x06
#define KEY_LEFT_7 0x07
#define KEY_LEFT_8 0x08
#define KEY_LEFT_9 0x09
// Right keys (Group 1)
#define KEY_RIGHT_1 0x11
#define KEY_RIGHT_2 0x12
#define KEY_RIGHT_3 0x13
#define KEY_RIGHT_4 0x14
#define KEY_RIGHT_5 0x15
#define KEY_RIGHT_6 0x16
#define KEY_RIGHT_7 0x17
#define KEY_RIGHT_8 0x18
#define KEY_RIGHT_9 0x19
// Other (Group 2)
#define KEY_L      0x21
#define KEY_R      0x22
#define KEY_LT     0x23
#define KEY_RT     0x24

class CapsuleChordKeypad {
public: class Key;
private:
    std::queue<char> _events;
    std::map<int,Key> keys;
public:
    void begin();
    void update();
    bool hasEvent();
    char getEvent();
    void disposeEvents();
    
    class Key {
        private:
            int id;
            bool mIsPressed = false;
        public:
            void press();
            void release();
            Key(){}
            Key(int keyId): id(keyId) {}
            bool isPressed();
    };

    Key operator [](int key){
        if(keys.find(key) == keys.end()) keys[key] = Key(key); // create if not exist
        return keys[key];
    }
};

extern CapsuleChordKeypad Keypad;

#endif