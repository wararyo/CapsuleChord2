#ifndef _CHORDKEYMAP_H_
#define _CHORDKEYMAP_H_

#include "Keypad.h"
#include "Chord.h"

// 左キーパッドの物理キーをnumberKeyMapテーブルに従ってコード演奏に変換するKeyMap。
// テーブル（物理キー→度数番号/センチネル）を差し替えることで
// 配列（Kantan/CapsuleChord等）を切り替えられる。
class ChordKeyMap : public CapsuleChordKeypad::KeyEventListener {
public:
    explicit ChordKeyMap(const uint8_t* numberKeyMap) : numberKeyMap(numberKeyMap) {}

    // KeyEventListenerインターフェース実装
    bool onKeyPressed(uint8_t keyCode) override;
    bool onKeyReleased(uint8_t keyCode) override;

    const uint8_t* getNumberKeyMap() const { return numberKeyMap; }
    void setNumberKeyMap(const uint8_t* table) { numberKeyMap = table; }

    // プリセットテーブル（9要素）
    static const uint8_t kantanNumberKeyMap[];
    static const uint8_t capsuleChordNumberKeyMap[];

private:
    const uint8_t* numberKeyMap;

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
