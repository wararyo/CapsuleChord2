#ifndef _KEYMAPBASE_H_
#define _KEYMAPBASE_H_

#include "Context.h"
#include "Keypad.h"

class KeyMapBase : public CapsuleChordKeypad::KeyEventListener {
protected:
    Context *context;

public:
    KeyMapBase(Context *context): context(context) {}
    virtual ~KeyMapBase() = default;
    bool onKeyPressed(uint8_t keyCode) override { return false; }
    bool onKeyReleased(uint8_t keyCode) override { return false; }
};

#endif