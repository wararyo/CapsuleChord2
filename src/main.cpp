#include <M5Unified.h>
#include <vector>
#include <lvgl.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/gpio.h>

static const char* LOG_TAG = "Main";

static inline unsigned long esp_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

static inline void esp_pinMode(gpio_num_t pin, gpio_mode_t mode) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = mode;
    io_conf.pull_up_en = (mode == GPIO_MODE_INPUT) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

static inline int esp_digitalRead(gpio_num_t pin) {
    return gpio_get_level(pin);
}
#include "BLEMidi.h"
#include "Chord.h"
#include "Scale.h"
#include "Keypad.h"
#include "KeyMap/KeyMap.h"
#include "Settings.h"
#include "Output/MidiOutput.h"
#include "Tempo.h"
#include "ChordPipeline.h"
#include "LvglWrapper.h"
#include "Widget/lv_chordlabel.h"
#include "Widget/lv_battery.h"
#include "Widget/lv_tickframe.h"
#include "Widget/TempoDialog.h"
#include "App/AppManager.h"
#include "App/AppSequencer.h"
#include "App/AppBass.h"
#include "App/AppAutoPlay.h"
#include "Widget/AppLauncher.h"
#include "Widget/PlayScreen.h"
#include "I2CHandler.h"
#include "LittleFSManager.h"

#define GPIO_NUM_BACK GPIO_NUM_7
#define GPIO_NUM_HOME GPIO_NUM_5
#define GPIO_NUM_MENU GPIO_NUM_8

unsigned long lastLoopMillis = 0;

m5::Button_Class BtnBack;
m5::Button_Class BtnHome;
m5::Button_Class BtnMenu;

PlayScreen playScreen;
AppLauncher appLauncher;

// PlayScreen表示状態の管理用
bool shouldShowPlayScreen = true;

// Initialize at setup()
Scale *scale;
int *centerNoteNo;
KeyMapBase *currentKeyMap;

typedef std::vector<const char *> strs;

// Helper function to create unique_ptr vector
std::vector<std::unique_ptr<SettingItem>> makeSettingsItems() {
    std::vector<std::unique_ptr<SettingItem>> items;
    items.push_back(std::make_unique<SettingItemEnum>("Mode",strs{"CapsuleChord","CAmDion","Presets"},0));
    items.push_back(std::make_unique<SettingItemScale>("Scale",Scale(0)));
    items.push_back(std::make_unique<SettingItemEnum>("Bass",strs{"None","C1","C2"},0));
    items.push_back(std::make_unique<SettingItemEnum>("Voicing",strs{"Open","Closed"},0));
    items.push_back(std::make_unique<SettingItemNumeric>("CenterNoteNo",24,81,60));
    items.push_back(std::make_unique<SettingItemDegreeChord>("Custom 1", DegreeChord(4,Chord::Minor|Chord::MajorSeventh)));
    items.push_back(std::make_unique<SettingItemDegreeChord>("Custom 2", DegreeChord(5,Chord::Minor|Chord::Seventh)));

    // Keymap with nested children
    std::vector<std::unique_ptr<SettingItem>> keymapChildren;
    keymapChildren.push_back(std::make_unique<SettingItemEnum>("Fuction 1",strs{"Gyro","Sustain","Note","CC"},0));
    keymapChildren.push_back(std::make_unique<SettingItemEnum>("Fuction 2",strs{"Gyro","Sustain","Note","CC"},1));
    items.push_back(std::make_unique<SettingItem>("Keymap",std::move(keymapChildren)));

    items.push_back(std::make_unique<SettingItemEnum>("SustainBehavior",strs{"Normal","Trigger"},0));
    items.push_back(std::make_unique<SettingItemEnum>("Brightness",strs{"Bright","Normal","Dark"},1));
    return items;
}

Settings settings(makeSettingsItems());

void update_battery() {
  if (playScreen.isShown()) {
    playScreen.updateBattery();
  }
}

void update_scale() {
  if (playScreen.isShown()) {
    playScreen.updateScale(scale->toString().c_str());
  }
}

void update_tempo() {
  if (playScreen.isShown()) {
    playScreen.updateTempo();
  }
}

// PlayScreenの表示状態を管理する関数
void updatePlayScreenVisibility() {
  bool shouldShow = !appLauncher.getShown() && (App.getCurrentApp() == nullptr);
  
  if (shouldShow && !playScreen.isShown()) {
    // PlayScreenを表示する必要があり、現在非表示の場合
    playScreen.create();
    // UI状態を更新
    update_battery();
    update_scale();
    update_tempo();
  } else if (!shouldShow && playScreen.isShown()) {
    // PlayScreenを非表示にする必要があり、現在表示中の場合
    playScreen.del();
  }
}

void setup() {
  M5.begin();
  
  M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  Lvgl.begin();

  BtnHome.setHoldThresh(2500);
  Keypad.begin();

  ESP_LOGI(LOG_TAG, "Hello.");

  I2C.begin();

  // Mount LittleFS (used for settings and timbre loading)
  if (!mountLittleFS()) {
    ESP_LOGW(LOG_TAG, "LittleFS mount failed. Settings will use defaults.");
  }

  // Load settings from LittleFS
  if (!settings.load()) {
    ESP_LOGI(LOG_TAG, "Settings file not found, creating with defaults.");
    if (!settings.save()) {
      ESP_LOGW(LOG_TAG, "Failed to save default settings.");
    }
  }

  // Get setting items
  scale = &((SettingItemScale*)settings.findSettingByKey("Scale"))->content;
  centerNoteNo = &((SettingItemNumeric*)settings.findSettingByKey("CenterNoteNo"))->number;

  // Set lcd brightness
  switch(((SettingItemEnum*)settings.findSettingByKey("Brightness"))->index) {
    case 0: M5.Lcd.setBrightness(255); break;
    case 1: M5.Lcd.setBrightness(127); break;
    case 2: M5.Lcd.setBrightness(32); break;
  }

  // Keymap initialization
  currentKeyMap = KeyMap::getAvailableKeyMaps()[0].get();
  
  // Register keymap as the bottom-level event listener
  std::shared_ptr<CapsuleChordKeypad::KeyEventListener> keyMapPtr(
      currentKeyMap, [](CapsuleChordKeypad::KeyEventListener*){} // 所有権を持たないカスタムデリータ
  );
  Keypad.addKeyEventListener(keyMapPtr);

  // 3ボタン
  esp_pinMode(GPIO_NUM_BACK, GPIO_MODE_INPUT);
  esp_pinMode(GPIO_NUM_HOME, GPIO_MODE_INPUT);
  esp_pinMode(GPIO_NUM_MENU, GPIO_MODE_INPUT);

  // 内蔵音源を開始（初期デバイスとして）
  Output.Internal.begin();

  // PlayScreen UI の初期化（内部でコード/テンポのコールバックも登録される）
  playScreen.create();

  // 初期UI状態の更新
  update_battery();
  update_scale();
  update_tempo();
}

void loop()
{
  // M5.update()とKeypad.update()はI2Cスレッドで処理される
  
  unsigned long ms = esp_millis();
  BtnBack.setRawState(ms, esp_digitalRead(GPIO_NUM_BACK) == 0);
  BtnHome.setRawState(ms, esp_digitalRead(GPIO_NUM_HOME) == 0);
  BtnMenu.setRawState(ms, esp_digitalRead(GPIO_NUM_MENU) == 0);

  if (BtnBack.wasPressed())
  {
    if(scale->key > 0) scale->key--;
    else scale->key = 11;
    update_scale();
  }
  if (BtnMenu.wasPressed())
  {
    if(scale->key < 11) scale->key++;
    else scale->key = 0;
    update_scale();
  }
  if (BtnBack.wasPressed() || BtnMenu.wasPressed())
  {
    Output.Internal.NoteOn(60 + scale->key, 100, 0);
  }
  else if (BtnBack.wasReleased() || BtnMenu.wasReleased())
  {
    Output.Internal.NoteOff(60 + scale->key, 0, 0);
  }

  // 出力デバイスの更新（ヘッドフォン検出など）
  Output.getCurrentOutput()->update();

  // 仮でホームボタンを押したらバッテリー残量が更新されるようにする
  if (BtnHome.wasPressed()) update_battery();
  // ホームボタン長押しでシーケンサー&ベース有効化 + 自動演奏開始 (展示用機能)
  if (BtnHome.wasHold()) {
    // 1. シーケンサーアプリとベースアプリを有効にする
    AppSequencer* sequencerApp = nullptr;
    AppBass* bassApp = nullptr;
    AppAutoPlay* autoPlayApp = nullptr;
    
    for (const auto& app : App.apps) {
      if (strcmp(app->getAppName(), "シーケンサー") == 0) {
        sequencerApp = static_cast<AppSequencer*>(app.get());
      } else if (strcmp(app->getAppName(), "ベース") == 0) {
        bassApp = static_cast<AppBass*>(app.get());
      } else if (strcmp(app->getAppName(), "自動演奏") == 0) {
        autoPlayApp = static_cast<AppAutoPlay*>(app.get());
      }
    }
    
    if (sequencerApp && bassApp && autoPlayApp) {
      if (autoPlayApp->getActive())
      {
          autoPlayApp->onDeactivate();
      }
      else
      {
        // シーケンサーアプリを有効化
        if (!sequencerApp->getActive()) {
          sequencerApp->onActivate();
        }
        
        // ベースアプリを有効化
        if (!bassApp->getActive()) {
          bassApp->onActivate();
        }
        
        // 2. シーケンサーのパターンをアコギにする
        sequencerApp->setGuitarPattern();
        
        // 3. 自動再生アプリを開く
        // App.launchApp(autoPlayApp);
        
        // 4. 再生を開始する
        if (!autoPlayApp->getActive()) {
          autoPlayApp->onActivate();
        }
      }
    }
  }

  // ホームボタンを押したらアプリ一覧
  if (BtnHome.wasPressed()) {
    if (playScreen.getTempoDialog().getShown()) {
      playScreen.getTempoDialog().del();
    }
    else if (App.getCurrentApp() != nullptr)
    {
      App.hideApp();
    } else if (appLauncher.getShown())
    {
      appLauncher.del();
    }
    else
    {
      appLauncher.create();
    }
  }

  // UI更新
  updatePlayScreenVisibility();
  
  if (appLauncher.getShown()) {
    appLauncher.update();
  }
  AppBase* currentApp = App.getCurrentApp();
  if (currentApp != nullptr) {
    currentApp->onUpdateGui();
  }
  playScreen.update();
  lv_task_handler();

  while(esp_millis() - lastLoopMillis < 5);
  lastLoopMillis = esp_millis();
}

// ESP-IDF entry point
extern "C" void app_main(void) {
  setup();
  while (true) {
    loop();
  }
}