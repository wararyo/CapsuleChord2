#include "AppAutoPlay.h"
#include "../Tempo.h"
#include "../ChordPipeline.h"
#include <algorithm>

extern TempoController Tempo;

void AppAutoPlay::onCreate()
{
    // 初期化処理
    tempoCallbacks.app = this;
    
    // 演奏状態を初期化
    resetPlayback();
    
    // LEDレイヤーを作成
    ledLayer = std::make_shared<LedLayer>("AutoPlay");
    
    // 自動演奏用のLEDパターンを設定
    setupLedPattern();
}

void AppAutoPlay::onActivate()
{
    if (isActive) return;
    
    startPlayback();
}

void AppAutoPlay::onDeactivate()
{
    if (!isActive) return;
    
    // 演奏停止
    stopPlayback();
}

void AppAutoPlay::onShowGui(lv_obj_t *container)
{
    if (isShowingGui) return;
    
    isShowingGui = true;
    
    // LEDレイヤーをアクティブにする
    if (context && context->keypad && ledLayer) {
        context->keypad->pushLedLayer(ledLayer);
    }
    
    // タイトルラベル
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, currentScore.title.c_str());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);
    
    // ステータスラベル
    statusLabel = lv_label_create(container);
    lv_label_set_text(statusLabel, "停止中");
    lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 35);
    
    // 現在のコードラベル
    currentChordLabel = lv_label_create(container);
    lv_label_set_text(currentChordLabel, "---");
    lv_obj_align(currentChordLabel, LV_ALIGN_TOP_MID, 0, 60);
    
    // 再生ボタン
    playButton = lv_btn_create(container);
    lv_obj_set_size(playButton, 80, 40);
    lv_obj_align(playButton, LV_ALIGN_CENTER, -50, -10);
    lv_obj_add_event_cb(playButton, playButtonEventHandler, LV_EVENT_CLICKED, this);
    
    lv_obj_t* playLabel = lv_label_create(playButton);
    lv_label_set_text(playLabel, "再生");
    lv_obj_center(playLabel);
    
    // 停止ボタン
    stopButton = lv_btn_create(container);
    lv_obj_set_size(stopButton, 80, 40);
    lv_obj_align(stopButton, LV_ALIGN_CENTER, 50, -10);
    lv_obj_add_event_cb(stopButton, stopButtonEventHandler, LV_EVENT_CLICKED, this);
    
    lv_obj_t* stopLabel = lv_label_create(stopButton);
    lv_label_set_text(stopLabel, "停止");
    lv_obj_center(stopLabel);
    
    // プログレスバー
    progressBar = lv_bar_create(container);
    lv_obj_set_size(progressBar, 200, 20);
    lv_obj_align(progressBar, LV_ALIGN_CENTER, 0, 40);
    lv_bar_set_range(progressBar, 0, 100);
    lv_bar_set_value(progressBar, 0, LV_ANIM_OFF);
    
    // 初期状態を更新
    updateStatus();
    updateProgress();
    updateCurrentChord();
}

void AppAutoPlay::onHideGui()
{
    if (!isShowingGui) return;
    
    // LEDレイヤーを非アクティブにする
    if (context && context->keypad && ledLayer) {
        context->keypad->removeLedLayer(ledLayer);
    }
    
    // UIオブジェクトを削除
    if (titleLabel) lv_obj_del(titleLabel);
    if (statusLabel) lv_obj_del(statusLabel);
    if (currentChordLabel) lv_obj_del(currentChordLabel);
    if (playButton) lv_obj_del(playButton);
    if (stopButton) lv_obj_del(stopButton);
    if (progressBar) lv_obj_del(progressBar);
    
    titleLabel = nullptr;
    statusLabel = nullptr;
    currentChordLabel = nullptr;
    playButton = nullptr;
    stopButton = nullptr;
    progressBar = nullptr;
    
    isShowingGui = false;
}

void AppAutoPlay::onDestroy()
{
    // 終了処理
    onDeactivate();
    onHideGui();
}

void AppAutoPlay::onUpdateGui()
{
    // 新しい時刻があるかチェック（割り込みからの更新）
    if (hasNewTime)
    {
        hasNewTime = false;
        musical_time_t currentTime = latestTime;
        
        // コマンドを処理（メインループで実行）
        processCommands(currentTime);
        
        // プログレス更新
        currentProgress = currentTime;
        needsProgressUpdate = true;
    }
    
    if (!isShowingGui) return;
    
    // フラグに基づいてUIを更新
    if (needsProgressUpdate)
    {
        updateProgress();
        needsProgressUpdate = false;
    }
    
    if (needsStatusUpdate)
    {
        updateStatus();
        needsStatusUpdate = false;
    }
    
    if (needsChordUpdate)
    {
        updateCurrentChord();
        needsChordUpdate = false;
    }
}

// Tempoコールバック実装
void AppAutoPlay::TempoCallbacks::onPlayingStateChanged(bool isPlaying)
{
    if (isPlaying)
    {
        app->previousTime = Tempo.getMusicalTime();
    }
    else
    {
        // 演奏停止時の処理
        app->resetPlayback();
    }
    
    app->needsStatusUpdate = true;
}

void AppAutoPlay::TempoCallbacks::onTempoChanged(TempoController::tempo_t tempo)
{
    // テンポ変更時の処理（必要に応じて実装）
}

void AppAutoPlay::TempoCallbacks::onTick(TempoController::tick_timing_t timing, musical_time_t time)
{
    if (!app->isActive) return;
    
    // 割り込み内では最小限の処理のみ行う
    app->latestTime = time;
    app->hasNewTime = true;
    
    app->previousTime = time;
}

// 演奏制御メソッド
void AppAutoPlay::startPlayback()
{
    isActive = true;
    // デフォルト譜面を読み込み
    loadDefaultScore();
    if (!Tempo.getPlaying())
    {
        // 譜面のテンポを設定
        Tempo.setTempo(currentScore.tempo);
        // Tempoコールバックを登録
        Tempo.addListener(&tempoCallbacks);
        // 演奏開始
        Tempo.play();
    }
}

void AppAutoPlay::stopPlayback()
{
    isActive = false;
    if (Tempo.getPlaying())
    {
        Tempo.stop();
        // Tempoコールバックを解除
        Tempo.removeListener(&tempoCallbacks);
    }
}

void AppAutoPlay::resetPlayback()
{
    nextCommandIndex = 0;
    currentProgress = 0;
    latestTime = 0;
    hasNewTime = false;
    needsProgressUpdate = true;
    needsStatusUpdate = true;
}

void AppAutoPlay::processCommands(musical_time_t currentTime)
{
    const size_t scoreSize = currentScore.commands.size();
    
    // 安全策: 1回の処理で最大50コマンドまで
    int processedCount = 0;
    const int MAX_COMMANDS_PER_UPDATE = 50;
    
    // 現在時刻までのコマンドを順次実行
    while (nextCommandIndex < scoreSize && processedCount < MAX_COMMANDS_PER_UPDATE)
    {
        const auto& command = currentScore.commands[nextCommandIndex];
        
        if (command.time <= currentTime)
        {
            Serial.printf("nextCommandIndex=%zu, command.time=%d, currentTime=%d\n", 
                          nextCommandIndex, command.time, currentTime);
            executeCommand(command);
            nextCommandIndex++;
            processedCount++;
        }
        else
        {
            break;
        }
    }
    
    // 処理制限に達した場合の警告
    if (processedCount >= MAX_COMMANDS_PER_UPDATE)
    {
        Serial.println("Warning: Command processing limit reached");
    }
    
    // ループ処理
    if (isLooping && nextCommandIndex >= currentScore.commands.size())
    {
        if (currentTime >= currentScore.duration)
        {
            // 譜面の最後まで到達したらリセット
            Tempo.stop();
            resetPlayback();
            if (Tempo.getPlaying())
            {
                Tempo.play();
            }
        }
    }
}

void AppAutoPlay::executeCommand(const AutoPlayCommand& command)
{
    if (!context || !context->pipeline) return;
    
    switch (command.type)
    {
        case CommandType::CHORD_START:
            Serial.printf("CHORD_START\n");
            // DegreeChordをChordに変換してから演奏開始
            {
                Chord chord = currentScore.degreeToChord(command.chordData.degreeChord);
                chord.calcInversion(*(uint8_t *)context->centerNoteNo);
                context->pipeline->playChord(chord);
                currentChord = chord;
                needsChordUpdate = true;
                
                // LEDを更新
                updateLedForCurrentChord();
            }
            break;
            
        case CommandType::CHORD_END:
            Serial.printf("CHORD_END\n");
            // コード演奏終了
            context->pipeline->stopChord();
            currentChord = std::nullopt; // コードなし
            needsChordUpdate = true;
            
            // LEDをクリア
            if (ledLayer) {
                ledLayer->fillLeds();
                if (context && context->keypad) {
                    context->keypad->markLedNeedsUpdate();
                }
            }
            break;
            
        case CommandType::MIDI_NOTE:
            Serial.printf("MIDI_NOTE: status=%02X, data1=%d, data2=%d\n", 
                          command.midiData.status, command.midiData.data1, command.midiData.data2);
            // MIDIノート送信
            if (command.midiData.status == 0x90) // Note On
            {
                context->pipeline->sendNote(true, command.midiData.data1, 
                                          command.midiData.data2, 9); // Channel 9 for drums
            }
            else if (command.midiData.status == 0x80) // Note Off
            {
                context->pipeline->sendNote(false, command.midiData.data1, 
                                          command.midiData.data2, 9);
            }
            break;
    }
}

// 譜面管理メソッド
void AppAutoPlay::loadDefaultScore()
{
    // 静的配列からデフォルト譜面を読み込み
    loadScoreFromArray(DemoSong::DEMO_COMMANDS, DemoSong::DEMO_COMMAND_COUNT, 
                      DemoSong::DEMO_TEMPO, "デモソング", DemoSong::DEMO_DURATION);
}

void AppAutoPlay::loadScoreFromArray(const AutoPlayCommand* commands, size_t commandCount, 
                                    TempoController::tempo_t tempo, const String& title, 
                                    musical_time_t duration)
{
    Serial.printf("Loading score: %s, tempo=%d, duration=%d, commandCount=%zu\n", 
                  title.c_str(), tempo, duration, commandCount);
    
    // MajorScaleを取得して設定（デフォルトでCメジャー）
    auto availableScales = Scale::getAvailableScales();
    ScaleBase* majorScale = nullptr;
    for (auto& scale : availableScales) {
        if (scale->name() == "Major") {
            majorScale = scale.get();
            break;
        }
    }
    
    // 譜面の基本情報を設定
    currentScore = Score(tempo, Chord::C, majorScale, title);
    currentScore.duration = duration;
    
    // コマンド配列をクリアして、新しいコマンドを追加
    currentScore.commands.clear();
    currentScore.commands.reserve(commandCount);
    
    // 配列からコマンドをコピー
    for (size_t i = 0; i < commandCount; i++)
    {
        currentScore.commands.push_back(commands[i]);
    }
    
    // コマンドを時間順にソート（配列が既にソート済みであっても安全のため）
    sortCommands();
}

void AppAutoPlay::sortCommands()
{
    std::sort(currentScore.commands.begin(), currentScore.commands.end(),
              [](const AutoPlayCommand& a, const AutoPlayCommand& b) {
                  return a.time < b.time;
              });
}

// UI更新メソッド
void AppAutoPlay::updateProgress()
{
    if (!progressBar) return;
    
    int progress = 0;
    if (currentScore.duration > 0)
    {
        progress = (currentProgress * 100) / currentScore.duration;
        progress = std::min(100, std::max(0, progress));
    }
    
    lv_bar_set_value(progressBar, progress, LV_ANIM_OFF);
}

void AppAutoPlay::updateStatus()
{
    if (!statusLabel) return;
    
    const char* status = Tempo.getPlaying() ? "再生中" : "停止中";
    lv_label_set_text(statusLabel, status);
}

void AppAutoPlay::updateCurrentChord()
{
    if (!currentChordLabel) return;
    
    String chordText;
    if (currentChord.has_value()) {
        chordText = currentChord->toString();
    } else {
        chordText = "---";
    }
    
    if (chordText.isEmpty()) {
        chordText = "---";
    }
    
    lv_label_set_text(currentChordLabel, chordText.c_str());
}

// イベントハンドラ
void AppAutoPlay::playButtonEventHandler(lv_event_t * e)
{
    AppAutoPlay* app = (AppAutoPlay*)lv_event_get_user_data(e);
    app->startPlayback();
}

void AppAutoPlay::stopButtonEventHandler(lv_event_t * e)
{
    AppAutoPlay* app = (AppAutoPlay*)lv_event_get_user_data(e);
    app->stopPlayback();
}

// LED管理メソッド
void AppAutoPlay::setupLedPattern()
{
    if (!ledLayer) return;
    
    ledLayer->fillLeds();
    
    if (context && context->keypad) {
        context->keypad->markLedNeedsUpdate();
    }
}

void AppAutoPlay::updateLedForCurrentChord()
{
    if (!ledLayer) return;
    
    // 現在のコードがない場合はクリア
    if (!currentChord.has_value())
    {
        setupLedPattern(); // 基本パターンに戻す
        return;
    }
    
    // 現在のコードに基づいてLEDパターンを設定
    std::map<uint8_t, uint8_t> chordLeds;
    
    // 現在演奏中のコードを示すために、コードの度数に対応するキーを光らせる
    // これは楽器の学習にも役立つ
    uint8_t scaleKey = currentScore.scaleKey;
    uint8_t chordRoot = currentChord->root;
    
    // スケールキーからの相対的な度数を計算
    int8_t degree = (chordRoot - scaleKey + 12) % 12;
    
    // 度数に対応するキーを明るく光らせる
    uint8_t keyCode = 0;
    switch (degree)
    {
        case 0:  keyCode = KEY_LEFT_7; break;  // I (1度)
        case 2:  keyCode = KEY_LEFT_8; break;  // II (2度)
        case 4:  keyCode = KEY_LEFT_9; break;  // III (3度)
        case 5:  keyCode = KEY_LEFT_4; break;  // IV (4度)
        case 7:  keyCode = KEY_LEFT_5; break;  // V (5度)
        case 9:  keyCode = KEY_LEFT_6; break;  // VI (6度)
        case 11: keyCode = KEY_LEFT_1; break;  // VII (7度)
        default: keyCode = KEY_LEFT_7; break;  // デフォルトは I
    }
    
    // 該当するキーを明るく光らせる
    if (keyCode != 0)
    {
        chordLeds[keyCode] = LED_BRIGHT;
    }
    
    // マイナーコードの場合は右手側のキーも光らせる
    if (currentChord->option & Chord::Minor)
    {
        chordLeds[KEY_RIGHT_8] = LED_BRIGHT; // マイナー表示
    }
    
    // セブンスコードの場合
    if (currentChord->option & (Chord::Seventh | Chord::MajorSeventh))
    {
        chordLeds[KEY_RIGHT_4] = LED_BRIGHT; // セブンス表示
    }

    // LEDを一括更新
    if (ledLayer) {
        ledLayer->setLeds(chordLeds);
        if (context && context->keypad) {
            context->keypad->markLedNeedsUpdate();
        }
    }
}
