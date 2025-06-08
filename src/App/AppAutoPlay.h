#pragma once

#include "AppBase.h"
#include "Tempo.h"
#include "Chord.h"
#include "Scale.h"
#include "Foundation/MusicalTime.h"
#include "../Keypad.h"
#include <vector>
#include <optional>

class AppAutoPlay : public AppBase
{
public:
    // 自動演奏コマンドの種類
    enum class CommandType : uint8_t
    {
        CHORD_START,    // コードの演奏開始
        CHORD_END,      // コードの演奏終了
        MIDI_NOTE       // MIDIノートの送信（ドラム演奏用）
    };

    // 自動演奏コマンド
    struct AutoPlayCommand
    {
        musical_time_t time;        // 実行タイミング
        CommandType type;           // コマンドの種類
        
        // コマンドデータ（union使用でメモリ効率化）
        union
        {
            // CHORD_START, CHORD_END用
            struct
            {
                DegreeChord degreeChord;        // 演奏するディグリーコード
            } chordData;
            
            // MIDI_NOTE用
            struct
            {
                uint8_t status;     // MIDIステータス（通常はNOTE_ON/NOTE_OFF）
                uint8_t data1;      // ノート番号
                uint8_t data2;      // ベロシティ
            } midiData;
        };
        
        // コンストラクタ
        AutoPlayCommand(musical_time_t time, const DegreeChord& degreeChord, CommandType type)
            : time(time), type(type)
        {
            chordData.degreeChord = degreeChord;
        }
        
        AutoPlayCommand(musical_time_t time, uint8_t status, uint8_t data1, uint8_t data2)
            : time(time), type(CommandType::MIDI_NOTE)
        {
            midiData.status = status;
            midiData.data1 = data1;
            midiData.data2 = data2;
        }
    };

    // 譜面データ
    struct Score
    {
        // メタデータ
        TempoController::tempo_t tempo;     // テンポ（BPM）
        uint8_t scaleKey;                   // 調のキー（C=0, C#=1, ...）
        ScaleBase* scale;                   // スケール（Major, Minor等）
        String title;                       // 楽曲タイトル
        
        // コマンドリスト（時間順にソート済み）
        std::vector<AutoPlayCommand> commands;
        
        // 譜面の長さ（musical_time_t）
        musical_time_t duration;
        
        // コンストラクタ
        Score() 
            : tempo(120), scaleKey(Chord::C), scale(nullptr), 
              title("Untitled"), duration(0) {}
              
        Score(TempoController::tempo_t tempo, uint8_t scaleKey, ScaleBase* scale, const String& title)
            : tempo(tempo), scaleKey(scaleKey), scale(scale), 
              title(title), duration(0) {}
              
        // DegreeChordをChordに変換するメソッド
        Chord degreeToChord(const DegreeChord& degreeChord) const
        {
            if (scale) {
                return scale->degreeToChord(scaleKey, degreeChord);
            } else {
                // スケールが設定されていない場合はそのまま変換
                return Chord(degreeChord.root, degreeChord.option, degreeChord.inversion);
            }
        }
    };

    // AppBase interface implementation
    char *getAppName() override { return "自動演奏"; }
    lv_img_dsc_t *getIcon() override { return nullptr; }
    bool runsInBackground() override { return true; }

    bool getActive() override { return isActive; }
    void onCreate() override;
    void onActivate() override;
    void onDeactivate() override;
    void onShowGui(lv_obj_t *container) override;
    void onHideGui() override;
    void onDestroy() override;
    void onUpdateGui() override;

private:
    // Tempoコールバック
    class TempoCallbacks : public TempoController::TempoCallbacks
    {
    public:
        AppAutoPlay *app;
        void onPlayingStateChanged(bool isPlaying) override;
        void onTempoChanged(TempoController::tempo_t tempo) override;
        void onTick(TempoController::tick_timing_t timing, musical_time_t time) override;
        TempoController::tick_timing_t getTimingMask() override
        {
            // すべてのタイミングで呼び出されるようにする（高精度な演奏のため）
            return TempoController::TICK_TIMING_BAR |
                   TempoController::TICK_TIMING_FULL |
                   TempoController::TICK_TIMING_FULL_TRIPLET |
                   TempoController::TICK_TIMING_HALF |
                   TempoController::TICK_TIMING_HALF_TRIPLET |
                   TempoController::TICK_TIMING_QUARTER |
                   TempoController::TICK_TIMING_EIGHTH;
        }
    };

    TempoCallbacks tempoCallbacks;
    bool isActive = false;
    bool isShowingGui = false;
    
    // LED Layer Management
    std::shared_ptr<LedLayer> ledLayer = nullptr;
    
    // 現在読み込まれている譜面
    Score currentScore;
    
    // 演奏状態
    musical_time_t previousTime = 0;    // 前回のTick時の時間
    size_t nextCommandIndex = 0;        // 次に実行するコマンドのインデックス
    bool isLooping = true;              // ループ再生するか
    
    // 割り込み処理の軽量化のための変数
    volatile musical_time_t latestTime = 0;  // 最新の時刻（割り込みから更新）
    volatile bool hasNewTime = false;        // 新しい時刻が更新されたか
    
    // UI要素
    lv_obj_t *titleLabel;
    lv_obj_t *statusLabel;
    lv_obj_t *currentChordLabel;
    lv_obj_t *playButton;
    lv_obj_t *stopButton;
    lv_obj_t *progressBar;
    
    // UI更新フラグ
    bool needsProgressUpdate = false;
    bool needsStatusUpdate = false;
    bool needsChordUpdate = false;
    musical_time_t currentProgress = 0;
    std::optional<Chord> currentChord;
    
    // 演奏制御メソッド
    void startPlayback();
    void stopPlayback();
    void resetPlayback();
    void processCommands(musical_time_t currentTime);
    void executeCommand(const AutoPlayCommand& command);
    
    // 譜面管理メソッド
    void loadDefaultScore();    // デフォルト譜面の読み込み
    void loadScoreFromArray(const AutoPlayCommand* commands, size_t commandCount, 
                           TempoController::tempo_t tempo, const String& title, 
                           musical_time_t duration); // 配列から譜面を読み込み
    void sortCommands();        // コマンドを時間順にソート
    
    // UI更新メソッド
    void updateProgress();
    void updateStatus();
    void updateCurrentChord();
    
    // LED管理メソッド
    void setupLedPattern();     // 自動演奏用のLEDパターンを設定
    void updateLedForCurrentChord(); // 現在のコードに基づいてLEDを更新
    
    // イベントハンドラ
    static void playButtonEventHandler(lv_event_t * e);
    static void stopButtonEventHandler(lv_event_t * e);
};

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
    
    constexpr size_t DEMO_COMMAND_COUNT = sizeof(DEMO_COMMANDS) / sizeof(DEMO_COMMANDS[0]);
    constexpr musical_time_t DEMO_DURATION = BAR_LENGTH * 8;
    constexpr TempoController::tempo_t DEMO_TEMPO = 110;
}
