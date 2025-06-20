#pragma once

#include "AppBase.h"
#include "Tempo.h"
#include "Chord.h"
#include "Scale.h"
#include "Foundation/MusicalTime.h"
#include "../Keypad.h"
#include <vector>
#include <optional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class AppAutoPlay : public AppBase
{
public:
    // KeyEventListener for blocking key events during autoplay
    class AutoPlayKeyListener : public CapsuleChordKeypad::KeyEventListener
    {
    public:
        bool onKeyPressed(uint8_t keyCode) override
        {
            // すべてのキーイベントを消費して自動演奏を保護
            return true;
        }
        
        bool onKeyReleased(uint8_t keyCode) override
        {
            // すべてのキーイベントを消費して自動演奏を保護
            return true;
        }
    };
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

    // 楽曲データ構造
    struct SongData
    {
        String title;
        const AutoPlayCommand* commands;
        size_t commandCount;
        TempoController::tempo_t tempo;
        musical_time_t duration;
        uint8_t key;                        // 楽曲のキー（C=0, C#=1, ...）
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
                //    TempoController::TICK_TIMING_FULL_TRIPLET |
                   TempoController::TICK_TIMING_HALF |
                //    TempoController::TICK_TIMING_HALF_TRIPLET |
                   TempoController::TICK_TIMING_QUARTER |
                   TempoController::TICK_TIMING_EIGHTH;
        }
    };

    TempoCallbacks tempoCallbacks;
    bool isActive = false;
    bool isShowingGui = false;
    
    // LED Layer Management
    std::shared_ptr<LedLayer> ledLayer = nullptr;
    
    // Key Event Listener for blocking key events during autoplay
    std::shared_ptr<AutoPlayKeyListener> keyListener = nullptr;
    
    // 現在読み込まれている譜面
    Score currentScore;
    
    // 楽曲選択関連
    std::vector<SongData> availableSongs;  // 利用可能な楽曲リスト
    size_t selectedSongIndex = 0;          // 現在選択されている楽曲のインデックス
    bool isContinuousMode = true;         // 連続再生モード
    
    // 演奏状態
    musical_time_t previousTime = 0;    // 前回のTick時の時間
    size_t nextCommandIndex = 0;        // 次に実行するコマンドのインデックス
    
    // 演奏タスク関連
    TaskHandle_t playbackTaskHandle = nullptr;     // 演奏タスクのハンドル
    volatile bool playbackTaskRunning = false;    // 演奏タスクが動作中かのフラグ
    
    // 割り込み処理の軽量化のための変数（廃止予定）
    volatile musical_time_t latestTime = 0;  // 最新の時刻（割り込みから更新）
    volatile bool hasNewTime = false;        // 新しい時刻が更新されたか
    
    // UI要素
    lv_obj_t *titleLabel;
    lv_obj_t *statusLabel;
    lv_obj_t *currentChordLabel;
    lv_obj_t *songSelectionDropdown;    // 楽曲選択ドロップダウン
    lv_obj_t *continuousModeSwitch;     // 連続再生モードスイッチ
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
    void executeCommand(const AutoPlayCommand& command);
    
    // 演奏タスク関連メソッド
    void createPlaybackTask();          // 演奏タスクを作成
    void destroyPlaybackTask();         // 演奏タスクを破棄
    static void playbackTaskWrapper(void* parameter);  // 演奏タスクのラッパー関数
    void playbackTaskMain();            // 演奏タスクのメイン処理
    void processCommands(musical_time_t currentTime); // タスク内でのコマンド処理
    
    // 譜面管理メソッド
    void initializeSongs();             // 楽曲データを初期化
    void loadSelectedScore();           // 選択された楽曲を読み込み
    void moveToNextSong();              // 次の曲に移動
    void loadScoreFromArray(const AutoPlayCommand* commands, size_t commandCount, 
                           TempoController::tempo_t tempo, const String& title, 
                           musical_time_t duration, uint8_t key = Chord::C); // 配列から譜面を読み込み（キー情報付き）
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
    static void songSelectionEventHandler(lv_event_t * e);  // 楽曲選択イベントハンドラ
    static void continuousModeEventHandler(lv_event_t * e); // 連続再生モードイベントハンドラ
};
