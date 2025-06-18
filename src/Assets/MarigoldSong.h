#pragma once

#include "../App/AppAutoPlay.h"
#include "DemoSong.h" // 基本定義を使用

// MarigoldSong (Dメジャーキー)
extern const AppAutoPlay::AutoPlayCommand MARIGOLDSONG_COMMANDS[];
extern const size_t MARIGOLDSONG_COMMAND_COUNT;
extern const musical_time_t MARIGOLDSONG_DURATION;
extern const TempoController::tempo_t MARIGOLDSONG_TEMPO;
extern const uint8_t MARIGOLDSONG_KEY;
