#ifndef _KEYMAP_H_
#define _KEYMAP_H_

#include <memory>
#include <vector>
#include "ChordKeyMap.h"

class KeyMap {
protected:
  static std::vector<std::shared_ptr<ChordKeyMap>> availableKeyMaps;
  static std::shared_ptr<ChordKeyMap> currentKeyMap;
public:
  static std::vector<std::shared_ptr<ChordKeyMap>> getAvailableKeyMaps() {
    if(availableKeyMaps.empty()) {
      availableKeyMaps.push_back(std::make_shared<ChordKeyMap>(ChordKeyMap::kantanNumberKeyMap));
      availableKeyMaps.push_back(std::make_shared<ChordKeyMap>(ChordKeyMap::capsuleChordNumberKeyMap));
    }
    return availableKeyMaps;
  }

  // 現在使用中のKeyMap。setup()で設定され、以降は差し替えられない前提
  // （将来切り替えを実装する場合はI2Cスレッドとの競合に注意）。
  static void setCurrentKeyMap(std::shared_ptr<ChordKeyMap> keyMap) { currentKeyMap = keyMap; }
  static std::shared_ptr<ChordKeyMap> getCurrentKeyMap() {
    if (!currentKeyMap) currentKeyMap = getAvailableKeyMaps()[0];
    return currentKeyMap;
  }
};

#endif
