#ifndef _CAPSULECHORDKEYMAP_H_
#define _CAPSULECHORDKEYMAP_H_

#include "KeyMapBase.h"
#include "Keypad.h"
#include "Chord.h"

class CapsuleChordKeyMap : public KeyMapBase {
public:
    using KeyMapBase::KeyMapBase;
    
    // KeyEventListenerインターフェース実装
    bool onKeyPressed(uint8_t keyCode) override;
    bool onKeyReleased(uint8_t keyCode) override;
    
    static const uint8_t numberKeyMap[]; //Key and diatonic chord matching
};

#endif