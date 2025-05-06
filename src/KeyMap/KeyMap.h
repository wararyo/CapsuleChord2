#ifndef _KEYMAP_H_
#define _KEYMAP_H_

#include <memory>
#include <vector>
#include "KeyMapBase.h"
#include "CapsuleChordKeyMap.h"
#include "KantanChordKeyMap.h"
#include "Context.h"

class KeyMap {
protected:
  static std::vector<std::shared_ptr<KeyMapBase>> availableKeyMaps;
public:
    static std::vector<std::shared_ptr<KeyMapBase>> getAvailableKeyMaps() {
    if(availableKeyMaps.empty()) {
      Context* ctx = Context::getContext();
      availableKeyMaps.push_back(std::make_shared<KantanChordKeyMap>(ctx));
      availableKeyMaps.push_back(std::make_shared<CapsuleChordKeyMap>(ctx));
    }
    return availableKeyMaps;
  }
};

#endif