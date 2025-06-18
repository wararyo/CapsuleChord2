#include "RockSong.h"

// ロック（2小節、I-V進行）
const AppAutoPlay::AutoPlayCommand ROCK_COMMANDS[] = {
    // === コード進行 ===
    // 1小節目: I (C Major)
    {0, DegreeChord(DegreeChord::I, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
    {BAR_LENGTH - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
    
    // 2小節目: V (G Major)
    {BAR_LENGTH, DegreeChord(DegreeChord::V, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
    {BAR_LENGTH * 2 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
    
    // === ドラムパターン（激しいロックパターン） ===
    // 1小節目 - より激しいパターン
    {0, 0x99, KICK, 127},
    {0, 0x99, CRASH, 127},
    {EIGHTH_NOTE, 0x99, KICK, 100},
    {QUARTER_NOTE, 0x99, SNARE, 120},
    {QUARTER_NOTE + EIGHTH_NOTE, 0x99, KICK, 100},
    {QUARTER_NOTE * 2, 0x99, KICK, 127},
    {QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x99, KICK, 100},
    {QUARTER_NOTE * 3, 0x99, SNARE, 120},
    {QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x99, KICK, 100},
    
    // 2小節目
    {BAR_LENGTH, 0x99, KICK, 127},
    {BAR_LENGTH + EIGHTH_NOTE, 0x99, KICK, 100},
    {BAR_LENGTH + QUARTER_NOTE, 0x99, SNARE, 120},
    {BAR_LENGTH + QUARTER_NOTE + EIGHTH_NOTE, 0x99, KICK, 100},
    {BAR_LENGTH + QUARTER_NOTE * 2, 0x99, KICK, 127},
    {BAR_LENGTH + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x99, KICK, 100},
    {BAR_LENGTH + QUARTER_NOTE * 3, 0x99, SNARE, 120},
    {BAR_LENGTH + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x99, KICK, 100},
};

const size_t ROCK_COMMAND_COUNT = sizeof(ROCK_COMMANDS) / sizeof(ROCK_COMMANDS[0]);
const musical_time_t ROCK_DURATION = BAR_LENGTH * 2;
const TempoController::tempo_t ROCK_TEMPO = 140;
