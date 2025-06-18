#include "SongData.h"

// サンプル譜面データ（8小節、C-Am-F-G進行）
namespace DemoSong {
    // 譜面データ配列
    const AppAutoPlay::AutoPlayCommand DEMO_COMMANDS[] = {
        // === コード進行 ===
        // 1小節目: I (C Major)
        {0, DegreeChord(DegreeChord::I, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
        {BAR_LENGTH - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
        
        // 2小節目: VI- (A Minor)
        {BAR_LENGTH, DegreeChord(DegreeChord::VI, Chord::Minor), AppAutoPlay::CommandType::CHORD_START},
        {BAR_LENGTH * 2 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
        
        // 3小節目: IV (F Major)
        {BAR_LENGTH * 2, DegreeChord(DegreeChord::IV, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
        {BAR_LENGTH * 3 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
        
        // 4小節目: V (G Major)
        {BAR_LENGTH * 3, DegreeChord(DegreeChord::V, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
        {BAR_LENGTH * 4 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
        
        // 5小節目: I (C Major) (繰り返し)
        {BAR_LENGTH * 4, DegreeChord(DegreeChord::I, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
        {BAR_LENGTH * 5 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
        
        // 6小節目: VI- (A Minor)
        {BAR_LENGTH * 5, DegreeChord(DegreeChord::VI, Chord::Minor), AppAutoPlay::CommandType::CHORD_START},
        {BAR_LENGTH * 6 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
        
        // 7小節目: IV (F Major)
        {BAR_LENGTH * 6, DegreeChord(DegreeChord::IV, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
        {BAR_LENGTH * 7 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
        
        // 8小節目: V (G Major)
        {BAR_LENGTH * 7, DegreeChord(DegreeChord::V, Chord::Major), AppAutoPlay::CommandType::CHORD_START},
        {BAR_LENGTH * 8 - 1, DegreeChord(), AppAutoPlay::CommandType::CHORD_END},
        
        // === ドラムパターン ===
        // 1小節目
        {0, 0x90, KICK, 100},                    // キック 1拍目
        {0, 0x90, CRASH, 100},                   // クラッシュ
        {0, 0x90, HIHAT, 80},                    // ハイハット
        {EIGHTH_NOTE, 0x90, HIHAT, 60},
        {QUARTER_NOTE, 0x90, SNARE, 90},         // スネア 2拍目
        {QUARTER_NOTE, 0x90, HIHAT, 80},
        {QUARTER_NOTE + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {QUARTER_NOTE * 2, 0x90, KICK, 100},     // キック 3拍目
        {QUARTER_NOTE * 2, 0x90, HIHAT, 80},
        {QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {QUARTER_NOTE * 3, 0x90, SNARE, 90},     // スネア 4拍目
        {QUARTER_NOTE * 3, 0x90, HIHAT, 80},
        {QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        
        // 2小節目
        {BAR_LENGTH, 0x90, KICK, 100},
        {BAR_LENGTH, 0x90, HIHAT, 80},
        {BAR_LENGTH + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH + QUARTER_NOTE, 0x90, SNARE, 90},
        {BAR_LENGTH + QUARTER_NOTE, 0x90, HIHAT, 80},
        {BAR_LENGTH + QUARTER_NOTE + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH + QUARTER_NOTE * 2, 0x90, KICK, 100},
        {BAR_LENGTH + QUARTER_NOTE * 2, 0x90, HIHAT, 80},
        {BAR_LENGTH + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH + QUARTER_NOTE * 3, 0x90, SNARE, 90},
        {BAR_LENGTH + QUARTER_NOTE * 3, 0x90, HIHAT, 80},
        {BAR_LENGTH + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        
        // 3小節目
        {BAR_LENGTH * 2, 0x90, KICK, 100},
        {BAR_LENGTH * 2, 0x90, HIHAT, 80},
        {BAR_LENGTH * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 2 + QUARTER_NOTE, 0x90, SNARE, 90},
        {BAR_LENGTH * 2 + QUARTER_NOTE, 0x90, HIHAT, 80},
        {BAR_LENGTH * 2 + QUARTER_NOTE + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 2 + QUARTER_NOTE * 2, 0x90, KICK, 100},
        {BAR_LENGTH * 2 + QUARTER_NOTE * 2, 0x90, HIHAT, 80},
        {BAR_LENGTH * 2 + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 2 + QUARTER_NOTE * 3, 0x90, SNARE, 90},
        {BAR_LENGTH * 2 + QUARTER_NOTE * 3, 0x90, HIHAT, 80},
        {BAR_LENGTH * 2 + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        
        // 4小節目
        {BAR_LENGTH * 3, 0x90, KICK, 100},
        {BAR_LENGTH * 3, 0x90, HIHAT, 80},
        {BAR_LENGTH * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 3 + QUARTER_NOTE, 0x90, SNARE, 90},
        {BAR_LENGTH * 3 + QUARTER_NOTE, 0x90, HIHAT, 80},
        {BAR_LENGTH * 3 + QUARTER_NOTE + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 3 + QUARTER_NOTE * 2, 0x90, KICK, 100},
        {BAR_LENGTH * 3 + QUARTER_NOTE * 2, 0x90, HIHAT, 80},
        {BAR_LENGTH * 3 + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 3 + QUARTER_NOTE * 3, 0x90, SNARE, 90},
        {BAR_LENGTH * 3 + QUARTER_NOTE * 3, 0x90, HIHAT, 80},
        {BAR_LENGTH * 3 + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        
        // 5小節目
        {BAR_LENGTH * 4, 0x90, KICK, 100},
        {BAR_LENGTH * 4, 0x90, HIHAT, 80},
        {BAR_LENGTH * 4 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 4 + QUARTER_NOTE, 0x90, SNARE, 90},
        {BAR_LENGTH * 4 + QUARTER_NOTE, 0x90, HIHAT, 80},
        {BAR_LENGTH * 4 + QUARTER_NOTE + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 4 + QUARTER_NOTE * 2, 0x90, KICK, 100},
        {BAR_LENGTH * 4 + QUARTER_NOTE * 2, 0x90, HIHAT, 80},
        {BAR_LENGTH * 4 + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 4 + QUARTER_NOTE * 3, 0x90, SNARE, 90},
        {BAR_LENGTH * 4 + QUARTER_NOTE * 3, 0x90, HIHAT, 80},
        {BAR_LENGTH * 4 + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        
        // 6小節目
        {BAR_LENGTH * 5, 0x90, KICK, 100},
        {BAR_LENGTH * 5, 0x90, HIHAT, 80},
        {BAR_LENGTH * 5 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 5 + QUARTER_NOTE, 0x90, SNARE, 90},
        {BAR_LENGTH * 5 + QUARTER_NOTE, 0x90, HIHAT, 80},
        {BAR_LENGTH * 5 + QUARTER_NOTE + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 5 + QUARTER_NOTE * 2, 0x90, KICK, 100},
        {BAR_LENGTH * 5 + QUARTER_NOTE * 2, 0x90, HIHAT, 80},
        {BAR_LENGTH * 5 + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 5 + QUARTER_NOTE * 3, 0x90, SNARE, 90},
        {BAR_LENGTH * 5 + QUARTER_NOTE * 3, 0x90, HIHAT, 80},
        {BAR_LENGTH * 5 + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        
        // 7小節目
        {BAR_LENGTH * 6, 0x90, KICK, 100},
        {BAR_LENGTH * 6, 0x90, HIHAT, 80},
        {BAR_LENGTH * 6 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 6 + QUARTER_NOTE, 0x90, SNARE, 90},
        {BAR_LENGTH * 6 + QUARTER_NOTE, 0x90, HIHAT, 80},
        {BAR_LENGTH * 6 + QUARTER_NOTE + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 6 + QUARTER_NOTE * 2, 0x90, KICK, 100},
        {BAR_LENGTH * 6 + QUARTER_NOTE * 2, 0x90, HIHAT, 80},
        {BAR_LENGTH * 6 + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 6 + QUARTER_NOTE * 3, 0x90, SNARE, 90},
        {BAR_LENGTH * 6 + QUARTER_NOTE * 3, 0x90, HIHAT, 80},
        {BAR_LENGTH * 6 + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        
        // 8小節目（最後）
        {BAR_LENGTH * 7, 0x90, KICK, 100},
        {BAR_LENGTH * 7, 0x90, CRASH, 100},              // 最後のクラッシュ
        {BAR_LENGTH * 7, 0x90, HIHAT, 80},
        {BAR_LENGTH * 7 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 7 + QUARTER_NOTE, 0x90, SNARE, 90},
        {BAR_LENGTH * 7 + QUARTER_NOTE, 0x90, HIHAT, 80},
        {BAR_LENGTH * 7 + QUARTER_NOTE + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 7 + QUARTER_NOTE * 2, 0x90, KICK, 100},
        {BAR_LENGTH * 7 + QUARTER_NOTE * 2, 0x90, HIHAT, 80},
        {BAR_LENGTH * 7 + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, HIHAT, 60},
        {BAR_LENGTH * 7 + QUARTER_NOTE * 3, 0x90, SNARE, 90},
        {BAR_LENGTH * 7 + QUARTER_NOTE * 3, 0x90, HIHAT, 80},
        {BAR_LENGTH * 7 + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, HIHAT, 60},
    };
    
    const size_t DEMO_COMMAND_COUNT = sizeof(DEMO_COMMANDS) / sizeof(DEMO_COMMANDS[0]);
    const musical_time_t DEMO_DURATION = BAR_LENGTH * 8;
    const TempoController::tempo_t DEMO_TEMPO = 110;
}

// 追加楽曲1: バラード（4小節、vi-IV-I-V進行）
namespace BalladSong {
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
        {0, 0x90, KICK, 80},                        // 1小節目
        {QUARTER_NOTE * 2, 0x90, SNARE, 70},
        
        {BAR_LENGTH, 0x90, KICK, 80},               // 2小節目
        {BAR_LENGTH + QUARTER_NOTE * 2, 0x90, SNARE, 70},
        
        {BAR_LENGTH * 2, 0x90, KICK, 80},           // 3小節目
        {BAR_LENGTH * 2 + QUARTER_NOTE * 2, 0x90, SNARE, 70},
        
        {BAR_LENGTH * 3, 0x90, KICK, 80},           // 4小節目
        {BAR_LENGTH * 3 + QUARTER_NOTE * 2, 0x90, SNARE, 70},
    };
    
    const size_t BALLAD_COMMAND_COUNT = sizeof(BALLAD_COMMANDS) / sizeof(BALLAD_COMMANDS[0]);
    const musical_time_t BALLAD_DURATION = BAR_LENGTH * 4;
    const TempoController::tempo_t BALLAD_TEMPO = 80;
}

// 追加楽曲2: ロック（2小節、I-V進行）
namespace RockSong {
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
        {0, 0x90, KICK, 127},
        {0, 0x90, CRASH, 127},
        {EIGHTH_NOTE, 0x90, KICK, 100},
        {QUARTER_NOTE, 0x90, SNARE, 120},
        {QUARTER_NOTE + EIGHTH_NOTE, 0x90, KICK, 100},
        {QUARTER_NOTE * 2, 0x90, KICK, 127},
        {QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, KICK, 100},
        {QUARTER_NOTE * 3, 0x90, SNARE, 120},
        {QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, KICK, 100},
        
        // 2小節目
        {BAR_LENGTH, 0x90, KICK, 127},
        {BAR_LENGTH + EIGHTH_NOTE, 0x90, KICK, 100},
        {BAR_LENGTH + QUARTER_NOTE, 0x90, SNARE, 120},
        {BAR_LENGTH + QUARTER_NOTE + EIGHTH_NOTE, 0x90, KICK, 100},
        {BAR_LENGTH + QUARTER_NOTE * 2, 0x90, KICK, 127},
        {BAR_LENGTH + QUARTER_NOTE * 2 + EIGHTH_NOTE, 0x90, KICK, 100},
        {BAR_LENGTH + QUARTER_NOTE * 3, 0x90, SNARE, 120},
        {BAR_LENGTH + QUARTER_NOTE * 3 + EIGHTH_NOTE, 0x90, KICK, 100},
    };
    
    const size_t ROCK_COMMAND_COUNT = sizeof(ROCK_COMMANDS) / sizeof(ROCK_COMMANDS[0]);
    const musical_time_t ROCK_DURATION = BAR_LENGTH * 2;
    const TempoController::tempo_t ROCK_TEMPO = 140;
}
