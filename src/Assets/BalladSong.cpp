#include "BalladSong.h"

// バラード（4小節、vi-IV-I-V進行）
const AppAutoPlay::AutoPlayCommand BALLAD_COMMANDS[] = {
    // === コード進行 ===
    // 1小節目: vi (A Minor)
    {0, DegreeChord(DegreeChord::VI, Chord::Minor), AppAutoPlay::CommandType::CHORD_START},
    {BAR_LENGTH - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
    
    // 2小節目: IV (F Major)
    {BAR_LENGTH, DegreeChord(DegreeChord::IV, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
    {BAR_LENGTH * 2 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
    
    // 3小節目: I (C Major)
    {BAR_LENGTH * 2, DegreeChord(DegreeChord::I, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
    {BAR_LENGTH * 3 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
    
    // 4小節目: V (G Major)
    {BAR_LENGTH * 3, DegreeChord(DegreeChord::V, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
    {BAR_LENGTH * 4 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
    
    // === ドラムパターン（よりシンプルなバラードパターン） ===
    // 各小節にキックとスネアのみ
    {0, 0x99, KICK, 80},                        // 1小節目
    {QUARTER_NOTE * 2, 0x99, SNARE, 70},
    
    {BAR_LENGTH, 0x99, KICK, 80},               // 2小節目
    {BAR_LENGTH + QUARTER_NOTE * 2, 0x99, SNARE, 70},
    
    {BAR_LENGTH * 2, 0x99, KICK, 80},           // 3小節目
    {BAR_LENGTH * 2 + QUARTER_NOTE * 2, 0x99, SNARE, 70},
    
    {BAR_LENGTH * 3, 0x99, KICK, 80},           // 4小節目
    {BAR_LENGTH * 3 + QUARTER_NOTE * 2, 0x99, SNARE, 70},
};

const size_t BALLAD_COMMAND_COUNT = sizeof(BALLAD_COMMANDS) / sizeof(BALLAD_COMMANDS[0]);
const musical_time_t BALLAD_DURATION = BAR_LENGTH * 4;
const TempoController::tempo_t BALLAD_TEMPO = 80;
