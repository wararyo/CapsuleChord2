#pragma once

#include "../App/AppAutoPlay.h"

// 基本的な時間定義
constexpr musical_time_t QUARTER_NOTE = 480;
constexpr musical_time_t EIGHTH_NOTE = 240;
constexpr musical_time_t BAR_LENGTH = 4 * QUARTER_NOTE;

// General MIDI ドラムマップ
constexpr uint8_t KICK = 36;
constexpr uint8_t SNARE = 38;
constexpr uint8_t HIHAT = 42;
constexpr uint8_t CRASH = 49;

// デモソング（8小節、C-Am-F-G進行）
extern const AppAutoPlay::AutoPlayCommand DEMO_COMMANDS[];
extern const size_t DEMO_COMMAND_COUNT;
extern const musical_time_t DEMO_DURATION;
extern const TempoController::tempo_t DEMO_TEMPO;
