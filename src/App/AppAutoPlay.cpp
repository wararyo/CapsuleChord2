#include "AppAutoPlay.h"
#include "AppManager.h"
#include "../Assets/DemoSong.h"
#include "../Assets/BalladSong.h"
#include "../Assets/RockSong.h"
#include "../Assets/MarigoldSong.h"
#include "../Tempo.h"
#include "../ChordPipeline.h"
#include "../Keypad.h"
#include "../SettingsStore.h"
#include <algorithm>
#include <esp_log.h>

static const char* LOG_TAG = "AppAutoPlay";

extern TempoController Tempo;

void AppAutoPlay::onCreate()
{
    // 初期化処理
    tempoCallbacks.app = this;
    
    // 楽曲データを初期化
    initializeSongs();
    
    // 演奏状態を初期化
    resetPlayback();
    
    // LEDレイヤーを作成
    ledLayer = std::make_shared<LedLayer>("AutoPlay");
    
    // KeyEventListenerを作成
    keyListener = std::make_shared<AutoPlayKeyListener>();
    
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
    
    // タイトルラベル
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);
    
    // ステータスラベル
    statusLabel = lv_label_create(container);
    lv_label_set_text(statusLabel, "停止中");
    lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 35);
    
    // 現在のコードラベル
    currentChordLabel = lv_label_create(container);
    lv_label_set_text(currentChordLabel, "---");
    lv_obj_align(currentChordLabel, LV_ALIGN_TOP_MID, 0, 60);
    
    // 楽曲選択ドロップダウン
    songSelectionDropdown = lv_dropdown_create(container);

    // 楽曲リストをドロップダウンに設定
    std::string songList = "";
    for (size_t i = 0; i < availableSongs.size(); i++) {
        if (i > 0) songList += "\n";
        songList += availableSongs[i].title;
    }
    lv_dropdown_set_options(songSelectionDropdown, songList.c_str());
    lv_dropdown_set_selected(songSelectionDropdown, selectedSongIndex);
    
    lv_obj_set_size(songSelectionDropdown, 150, 35);
    lv_obj_align(songSelectionDropdown, LV_ALIGN_TOP_MID, 0, 85);
    lv_obj_add_event_cb(songSelectionDropdown, songSelectionEventHandler, LV_EVENT_VALUE_CHANGED, this);
    
    // 連続再生モードスイッチ
    continuousModeSwitch = lv_switch_create(container);
    lv_obj_set_size(continuousModeSwitch, 50, 25);
    lv_obj_align(continuousModeSwitch, LV_ALIGN_TOP_MID, 0, 125);
    lv_obj_add_event_cb(continuousModeSwitch, continuousModeEventHandler, LV_EVENT_VALUE_CHANGED, this);
    if (isContinuousMode) lv_obj_add_state(continuousModeSwitch, LV_STATE_CHECKED);
    else lv_obj_clear_state(continuousModeSwitch, LV_STATE_CHECKED);
    
    // 連続再生モードラベル
    lv_obj_t* continuousLabel = lv_label_create(container);
    lv_label_set_text(continuousLabel, "連続再生");
    lv_obj_align(continuousLabel, LV_ALIGN_TOP_MID, 0, 155);
    
    // 再生ボタン
    playButton = lv_btn_create(container);
    lv_obj_set_size(playButton, 80, 40);
    lv_obj_align(playButton, LV_ALIGN_CENTER, -50, 50);
    lv_obj_add_event_cb(playButton, playButtonEventHandler, LV_EVENT_CLICKED, this);
    
    lv_obj_t* playLabel = lv_label_create(playButton);
    lv_label_set_text(playLabel, "再生");
    lv_obj_center(playLabel);
    
    // 停止ボタン
    stopButton = lv_btn_create(container);
    lv_obj_set_size(stopButton, 80, 40);
    lv_obj_align(stopButton, LV_ALIGN_CENTER, 50, 50);
    lv_obj_add_event_cb(stopButton, stopButtonEventHandler, LV_EVENT_CLICKED, this);
    
    lv_obj_t* stopLabel = lv_label_create(stopButton);
    lv_label_set_text(stopLabel, "停止");
    lv_obj_center(stopLabel);
    
    // プログレスバー
    progressBar = lv_bar_create(container);
    lv_obj_set_size(progressBar, 200, 20);
    lv_obj_align(progressBar, LV_ALIGN_CENTER, 0, 100);
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
    
    // UIオブジェクトを削除
    if (titleLabel) lv_obj_del(titleLabel);
    if (statusLabel) lv_obj_del(statusLabel);
    if (currentChordLabel) lv_obj_del(currentChordLabel);
    if (songSelectionDropdown) lv_obj_del(songSelectionDropdown);
    if (continuousModeSwitch) lv_obj_del(continuousModeSwitch);
    if (playButton) lv_obj_del(playButton);
    if (stopButton) lv_obj_del(stopButton);
    if (progressBar) lv_obj_del(progressBar);
    
    titleLabel = nullptr;
    statusLabel = nullptr;
    currentChordLabel = nullptr;
    songSelectionDropdown = nullptr;
    continuousModeSwitch = nullptr;
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
    
    // KeyEventListenerをクリーンアップ
    if (keyListener) {
        Keypad.removeKeyEventListener(keyListener);
    }
    keyListener = nullptr;
}

void AppAutoPlay::onUpdateGui()
{
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
    if (!app->isActive || !app->playbackTaskRunning) return;
    
    // タスク通知を使って演奏タスクに時刻を送信
    if (app->playbackTaskHandle != nullptr)
    {
        // 時刻をnotification valueとして直接送信
        xTaskNotifyFromISR(app->playbackTaskHandle, (uint32_t)time, eSetValueWithOverwrite, nullptr);
    }
    
    app->previousTime = time;
}

// 演奏制御メソッド
void AppAutoPlay::startPlayback()
{
    isActive = true;

    // KeyEventListenerを登録して自動演奏を保護
    if (keyListener) {
        Keypad.addKeyEventListener(keyListener);
        ESP_LOGI(LOG_TAG, "KeyEventListener registered");
    }

    // 演奏タスクを開始
    playbackTaskRunning = true;
    createPlaybackTask();

    // 選択された譜面を読み込み
    loadSelectedScore();
    if (!Tempo.getPlaying())
    {
        // 譜面のテンポを設定
        Tempo.setTempo(currentScore.tempo);
        // Tempoコールバックを登録
        Tempo.addListener(&tempoCallbacks);
        // 演奏開始
        Tempo.play();
    }

    // LEDレイヤーをアクティブにする
    if (ledLayer) {
        Keypad.pushLedLayer(ledLayer);
    }
}

void AppAutoPlay::stopPlayback()
{
    isActive = false;

    // KeyEventListenerを解除
    if (keyListener) {
        Keypad.removeKeyEventListener(keyListener);
        ESP_LOGI(LOG_TAG, "KeyEventListener unregistered");
    }

    // 演奏タスクを停止
    playbackTaskRunning = false;
    if (playbackTaskHandle != nullptr)
    {
        vTaskDelete(playbackTaskHandle);
        playbackTaskHandle = nullptr;
    }
    
    if (Tempo.getPlaying())
    {
        // Tempo.stop();
        // Tempoコールバックを解除
        Tempo.removeListener(&tempoCallbacks);
    }

    // LEDレイヤーを非アクティブにする
    if (ledLayer) {
        Keypad.removeLedLayer(ledLayer);
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

void AppAutoPlay::executeCommand(const AutoPlayCommand& command)
{
    switch (command.type)
    {
        case CommandType::CHORD_START:
            // Serial.printf("CHORD_START\n");
            // DegreeChordをChordに変換してから演奏開始
            {
                Chord chord = currentScore.degreeToChord(command.chordData.degreeChord);
                chord.calcInversion((uint8_t)Settings.voicing.centerNoteNo.get());
                Pipeline.playChord(chord);
                currentChord = chord;
                needsChordUpdate = true;
                
                // LEDを更新
                updateLedForCurrentChord();
            }
            break;
            
        case CommandType::CHORD_END:
            // Serial.printf("CHORD_END\n");QUARTER=136.36
            // コード演奏終了
            Pipeline.stopChord();
            currentChord = std::nullopt; // コードなし
            needsChordUpdate = true;
            
            // LEDをクリア
            if (ledLayer) {
                ledLayer->fillLeds();
                Keypad.markLedNeedsUpdate();
            }
            break;
            
        case CommandType::MIDI_NOTE:
            // Serial.printf("MIDI_NOTE: status=%02X, data1=%d, data2=%d millis()=%lu\n", 
            //               command.midiData.status, command.midiData.data1, command.midiData.data2, millis());
            // MIDIノート送信
            if ((command.midiData.status & 0xF0) == 0x90) // Note On
            {
                uint8_t channel = command.midiData.status & 0x0F;
                Pipeline.sendNote(true, command.midiData.data1, 
                                          command.midiData.data2, channel); // Channel 9 for drums
            }
            else if ((command.midiData.status & 0xF0) == 0x80) // Note Off
            {
                uint8_t channel = command.midiData.status & 0x0F;
                Pipeline.sendNote(false, command.midiData.data1, 
                                          command.midiData.data2, channel);
            }
            break;
    }
}

// 譜面管理メソッド
void AppAutoPlay::initializeSongs()
{
    // 利用可能な楽曲を初期化
    availableSongs.clear();
    
    // // デモソング
    // availableSongs.push_back({
    //     "デモソング",
    //     DEMO_COMMANDS,
    //     DEMO_COMMAND_COUNT,
    //     DEMO_TEMPO,
    //     DEMO_DURATION,
    //     Chord::C  // Cメジャー
    // });
    
    // // バラード
    // availableSongs.push_back({
    //     "バラード",
    //     BALLAD_COMMANDS,
    //     BALLAD_COMMAND_COUNT,
    //     BALLAD_TEMPO,
    //     BALLAD_DURATION,
    //     Chord::C  // Cメジャー
    // });
    
    // // ロック
    // availableSongs.push_back({
    //     "ロック",
    //     ROCK_COMMANDS,
    //     ROCK_COMMAND_COUNT,
    //     ROCK_TEMPO,
    //     ROCK_DURATION,
    //     Chord::C  // Cメジャー
    // });
    
    // マリーゴールド
    availableSongs.push_back({
        "マリーゴールド",
        MARIGOLDSONG_COMMANDS,
        MARIGOLDSONG_COMMAND_COUNT,
        MARIGOLDSONG_TEMPO,
        MARIGOLDSONG_DURATION,
        MARIGOLDSONG_KEY  // Dメジャー（外部定義から取得）
    });
    
    // 初期選択は最初の楽曲
    selectedSongIndex = 0;
}

void AppAutoPlay::loadSelectedScore()
{
    if (selectedSongIndex >= availableSongs.size()) {
        selectedSongIndex = 0; // 安全のため
    }
    
    const SongData& song = availableSongs[selectedSongIndex];
    
    // 選択された楽曲を読み込み
    loadScoreFromArray(song.commands, song.commandCount, 
                      song.tempo, song.title, song.duration, song.key);
}

void AppAutoPlay::loadScoreFromArray(const AutoPlayCommand* commands, size_t commandCount,
                                    TempoController::tempo_t tempo, const std::string& title,
                                    musical_time_t duration, uint8_t key)
{
    ESP_LOGI(LOG_TAG, "Loading score: %s, tempo=%d, duration=%d, commandCount=%zu, key=%d",
             title.c_str(), tempo, (int)duration, commandCount, key);
    
    // MajorScaleを取得して設定
    auto availableScales = Scale::getAvailableScales();
    ScaleBase* majorScale = nullptr;
    for (auto& scale : availableScales) {
        if (scale->name() == "Major") {
            majorScale = scale.get();
            break;
        }
    }
    
    // 譜面の基本情報を設定
    currentScore = Score(tempo, key, majorScale, title);
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
    
    // グローバルのscaleにキー情報を反映
    Scale newScale = Settings.performance.scale.get();
    newScale.key = key;
    Settings.performance.scale.set(newScale);
    ESP_LOGI(LOG_TAG, "Updated global scale key to: %d (%s)", key, Chord::rootStrings[key].c_str());
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

    std::string chordText;
    if (currentChord.has_value()) {
        chordText = currentChord->toString();
    } else {
        chordText = "---";
    }

    if (chordText.empty()) {
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

void AppAutoPlay::songSelectionEventHandler(lv_event_t * e)
{
    AppAutoPlay* app = (AppAutoPlay*)lv_event_get_user_data(e);
    lv_obj_t* dropdown = lv_event_get_target(e);

    app->selectedSongIndex = lv_dropdown_get_selected(dropdown);
    ESP_LOGI(LOG_TAG, "Song selection changed to index: %zu", app->selectedSongIndex);
    
    // 楽曲が変更されたら、演奏を停止して新しい楽曲を読み込む
    if (app->isActive) {
        app->stopPlayback();
        app->loadSelectedScore();  // 新しい楽曲のキー情報も反映される
    }
}

void AppAutoPlay::continuousModeEventHandler(lv_event_t * e)
{
    AppAutoPlay* app = (AppAutoPlay*)lv_event_get_user_data(e);
    lv_obj_t* sw = lv_event_get_target(e);

    app->isContinuousMode = lv_obj_has_state(sw, LV_STATE_CHECKED);
    ESP_LOGI(LOG_TAG, "Continuous mode: %s", app->isContinuousMode ? "ON" : "OFF");
}

// LED管理メソッド
void AppAutoPlay::setupLedPattern()
{
    if (!ledLayer) return;
    
    ledLayer->fillLeds();
    
    Keypad.markLedNeedsUpdate();
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
        Keypad.markLedNeedsUpdate();
    }
}

void AppAutoPlay::moveToNextSong()
{
    if (availableSongs.empty()) return;
    
    // 次の曲のインデックスを計算
    selectedSongIndex = (selectedSongIndex + 1) % availableSongs.size();
    
    // 新しい楽曲を読み込み
    loadSelectedScore();
    
    // UIのドロップダウンも更新
    if (songSelectionDropdown && isShowingGui) {
        lv_dropdown_set_selected(songSelectionDropdown, selectedSongIndex);
    }
    
    ESP_LOGI(LOG_TAG, "Next song: %s", currentScore.title.c_str());
}

// 演奏タスク関連メソッド
void AppAutoPlay::createPlaybackTask()
{
    if (playbackTaskHandle != nullptr) {
        // 既にタスクが存在する場合は何もしない
        return;
    }
    
    // 演奏タスクを作成（優先度は高めに設定）
    BaseType_t result = xTaskCreatePinnedToCore(
        playbackTaskWrapper,          // タスク関数
        "AutoPlayTask",               // タスク名
        4096,                         // スタックサイズ
        this,                         // パラメータ（thisポインタ）
        3,                            // 優先度（高め）
        &playbackTaskHandle,          // タスクハンドル
        1
    );
    
    if (result != pdPASS) {
        ESP_LOGE(LOG_TAG, "Failed to create playback task");
        playbackTaskHandle = nullptr;
    } else {
        ESP_LOGI(LOG_TAG, "Playback task created successfully");
    }
}

void AppAutoPlay::destroyPlaybackTask()
{
    if (playbackTaskHandle == nullptr) {
        return;
    }
    
    // タスクの実行を停止
    playbackTaskRunning = false;
    
    // タスクに通知を送って終了させる
    xTaskNotify(playbackTaskHandle, 0, eNoAction);
    
    // 少し待ってからタスクを削除
    vTaskDelay(pdMS_TO_TICKS(50));
    
    if (eTaskGetState(playbackTaskHandle) != eDeleted) {
        vTaskDelete(playbackTaskHandle);
    }
    
    playbackTaskHandle = nullptr;
    ESP_LOGI(LOG_TAG, "Playback task destroyed");
}

void AppAutoPlay::playbackTaskWrapper(void* parameter)
{
    AppAutoPlay* app = static_cast<AppAutoPlay*>(parameter);
    app->playbackTaskMain();
    
    // タスクが終了する場合はハンドルをクリア
    app->playbackTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

void AppAutoPlay::playbackTaskMain()
{
    ESP_LOGI(LOG_TAG, "Playback task started");
    
    uint32_t notificationValue = 0;
    musical_time_t currentTime = 0;
    
    while (playbackTaskRunning) {
        // タスク通知を待機（最大100ms）
        if (xTaskNotifyWait(0, ULONG_MAX, &notificationValue, pdMS_TO_TICKS(100)) == pdTRUE) {
            // 通知を受信した場合、時刻を取得
            currentTime = (musical_time_t)notificationValue;
            
            // コマンドを処理
            processCommands(currentTime);
            
            // プログレス更新（UIスレッドで処理するためフラグを設定）
            currentProgress = currentTime;
            needsProgressUpdate = true;
        }
        
        // タスクが停止指示を受けた場合は終了
        if (!playbackTaskRunning) {
            break;
        }
    }
    
    ESP_LOGI(LOG_TAG, "Playback task ended");
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
            // Serial.printf("Task: nextCommandIndex=%zu, command.time=%d, currentTime=%d\n", 
            //               nextCommandIndex, command.time, currentTime);
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
        ESP_LOGW(LOG_TAG, "Command processing limit reached in task");
    }
    
    // ループ処理
    if (nextCommandIndex >= currentScore.commands.size())
    {
        if (currentTime >= currentScore.duration)
        {
            // 譜面の最後まで到達
            if (isContinuousMode)
            {
                // 連続再生モードの場合、次の曲に移動
                moveToNextSong();
                // 新しい曲を開始
                Tempo.stop();
                resetPlayback();
                Tempo.setTempo(currentScore.tempo);
                Tempo.play();
            }
        }
    }
}
