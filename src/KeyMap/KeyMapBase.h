#ifndef _KEYMAPBASE_H_
#define _KEYMAPBASE_H_

#include "Keypad.h"

class KeyMapBase : public CapsuleChordKeypad::KeyEventListener {
public:
    KeyMapBase() {}
    virtual ~KeyMapBase() = default;
    bool onKeyPressed(uint8_t keyCode) override { return false; }
    bool onKeyReleased(uint8_t keyCode) override { return false; }
};

#endif