#ifndef _KANTANCHORDKEYMAP_H_
#define _KANTANCHORDKEYMAP_H_

#include "KeyMapBase.h"
#include "Keypad.h"
#include "Chord.h"

class KantanChordKeyMap : public KeyMapBase {
public:
    using KeyMapBase::KeyMapBase;
    void update() override;
    static const uint8_t numberKeyMap[]; //Key and diatonic chord matching
private:
    // 今なっているChordが鳴るきっかけとなった物理ボタン
    // ナンバーキーとして設定された任意の2つのボタンAとBについて、
    // 下記の順序で操作を行なったときに4の段階で音が止まるようにする必要がある
    // 1. Aを押す
    // 2. Bを押す
    // 3. Aを離す
    // 4. Bを離す
    // その制御のためにこの変数を用いる
    uint8_t currentPressingButton = 0;
};

#endif