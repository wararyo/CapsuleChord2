#pragma once

#include "../App/AppAutoPlay.h"
#include "DemoSong.h" // 基本定義を使用

// ロック（2小節、I-V進行）
extern const AppAutoPlay::AutoPlayCommand ROCK_COMMANDS[];
extern const size_t ROCK_COMMAND_COUNT;
extern const musical_time_t ROCK_DURATION;
extern const TempoController::tempo_t ROCK_TEMPO;
