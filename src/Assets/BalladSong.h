#pragma once

#include "../App/AppAutoPlay.h"
#include "DemoSong.h" // 基本定義を使用

// バラード（4小節、vi-IV-I-V進行）
extern const AppAutoPlay::AutoPlayCommand BALLAD_COMMANDS[];
extern const size_t BALLAD_COMMAND_COUNT;
extern const musical_time_t BALLAD_DURATION;
extern const TempoController::tempo_t BALLAD_TEMPO;
