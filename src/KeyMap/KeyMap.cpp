#include "KeyMap.h"

std::vector<std::shared_ptr<ChordKeyMap>> KeyMap::availableKeyMaps;
std::shared_ptr<ChordKeyMap> KeyMap::currentKeyMap;
