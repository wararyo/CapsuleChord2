#pragma once

#include "../App/AppAutoPlay.h"

// サンプル譜面データ（8小節、C-Am-F-G進行）
namespace DemoSong {
    // 基本的な時間定義
    constexpr musical_time_t QUARTER_NOTE = 480;
    constexpr musical_time_t EIGHTH_NOTE = 240;
    constexpr musical_time_t BAR_LENGTH = 4 * QUARTER_NOTE;
    
    // General MIDI ドラムマップ
    constexpr uint8_t KICK = 36;
    constexpr uint8_t SNARE = 38;
    constexpr uint8_t HIHAT = 42;
    constexpr uint8_t CRASH = 49;
    
    // 譜面データ配列
    extern const AppAutoPlay::AutoPlayCommand DEMO_COMMANDS[];
    extern const size_t DEMO_COMMAND_COUNT;
    extern const musical_time_t DEMO_DURATION;
    extern const TempoController::tempo_t DEMO_TEMPO;
}

// 追加楽曲1: バラード（4小節、vi-IV-I-V進行）
namespace BalladSong {
    using namespace DemoSong; // 基本定義を再利用
    
    extern const AppAutoPlay::AutoPlayCommand BALLAD_COMMANDS[];
    extern const size_t BALLAD_COMMAND_COUNT;
    extern const musical_time_t BALLAD_DURATION;
    extern const TempoController::tempo_t BALLAD_TEMPO;
}

// 追加楽曲2: ロック（2小節、I-V進行）
namespace RockSong {
    using namespace DemoSong; // 基本定義を再利用
    
    extern const AppAutoPlay::AutoPlayCommand ROCK_COMMANDS[];
    extern const size_t ROCK_COMMAND_COUNT;
    extern const musical_time_t ROCK_DURATION;
    extern const TempoController::tempo_t ROCK_TEMPO;
}
