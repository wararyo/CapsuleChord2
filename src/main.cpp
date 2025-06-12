#include <M5Unified.h>
#include <vector>
#include <Preferences.h>
#include <lvgl.h>
#include "BLEMidi.h"
#include "Chord.h"
#include "Scale.h"
#include "Keypad.h"
#include "KeyMap/KeyMap.h"
#include "Context.h"
#include "Output/MidiOutput.h"
#include "Tempo.h"
#include "ChordPipeline.h"
#include "LvglWrapper.h"
#include "Widget/lv_chordlabel.h"
#include "Widget/lv_battery.h"
#include "Widget/lv_tickframe.h"
#include "Widget/TempoDialog.h"
#include "App/AppManager.h"
#include "Widget/AppLauncher.h"
#include "Widget/PlayScreen.h"
#include "I2CHandler.h"

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
Context context;
KeyMapBase *currentKeyMap;

typedef std::vector<SettingItem*> si;
typedef std::vector<const char *> strs;
Settings settings(si{
  new SettingItemEnum("Mode",{"CapsuleChord","CAmDion","Presets"},0),
  new SettingItemScale("Scale",Scale(0)),
  new SettingItemEnum("Bass",{"None","C1","C2"},0),
  new SettingItemEnum("Voicing",{"Open","Closed"},0),
  new SettingItemNumeric("CenterNoteNo",24,81,60),
  new SettingItemDegreeChord("Custom 1", DegreeChord(4,Chord::Minor|Chord::MajorSeventh)),
  new SettingItemDegreeChord("Custom 2", DegreeChord(5,Chord::Minor|Chord::Seventh)),
  new SettingItem("Keymap",si{
    new SettingItemEnum("Fuction 1",{"Gyro","Sustain","Note","CC"},0),
    new SettingItemEnum("Fuction 2",{"Gyro","Sustain","Note","CC"},1)
  }),
  new SettingItemEnum("SustainBehavior",{"Normal","Trigger"},0),
  new SettingItemEnum("Brightness", {"Bright","Normal","Dark"},1)
});

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

// TickFrameの更新フラグ
bool needsTickUpdate = false;
TempoController::tick_timing_t lastTickTiming = 0;

class MainTempoCallbacks: public TempoController::TempoCallbacks {
    void onPlayingStateChanged(bool isPlaying) override
    {
    }
    void onTempoChanged(TempoController::tempo_t tempo) override {
      update_tempo();
    }
    void onTick(TempoController::tick_timing_t timing, musical_time_t time) override {
      // TickFrameの更新フラグをセット
      needsTickUpdate = true;
      lastTickTiming = timing;
    }
    TempoController::tick_timing_t getTimingMask() override {
      return TempoController::TICK_TIMING_BAR | TempoController::TICK_TIMING_FULL;
    }
};

class MainChordFilter: public ChordPipeline::ChordFilter {
    bool modifiesChord() override
    {
      return false;
    }
    void onChordOn(Chord chord) override
    {
      if (playScreen.isShown()) {
        playScreen.setChord(chord);
      }
    }
    void onChordOff() override
    {
    }
};

void setup() {
  M5.begin();
  
  M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  Lvgl.begin();

  BtnHome.setHoldThresh(3000);
  Keypad.begin();

  Serial.begin(115200);
  Serial.println("Hello.");

  I2C.begin();

  // Load settings
  // if(!settings.load()){
  //   Serial.println("settings.json is not found in SD, so I'll try to create it.");
  //   if(!settings.save()) Serial.println("Setting file creation failed.");
  // }

  // Get setting items
  scale = &((SettingItemScale*)settings.findSettingByKey(String("Scale")))->content;
  centerNoteNo = &((SettingItemNumeric*)settings.findSettingByKey(String("CenterNoteNo")))->number;

  // Set lcd brightness
  switch(((SettingItemEnum*)settings.findSettingByKey(String("Brightness")))->index) {
    case 0: M5.Lcd.setBrightness(255); break;
    case 1: M5.Lcd.setBrightness(127); break;
    case 2: M5.Lcd.setBrightness(32); break;
  }

  // Make Context
  context = Context(&settings, &Pipeline, &Keypad);
  Context::setContext(&context);
  
  // Initialize context in all apps
  App.initContext(&context);

  // Keymap initialization
  currentKeyMap = KeyMap::getAvailableKeyMaps()[0].get();
  
  // Register keymap as the bottom-level event listener
  std::shared_ptr<CapsuleChordKeypad::KeyEventListener> keyMapPtr(
      currentKeyMap, [](CapsuleChordKeypad::KeyEventListener*){} // 所有権を持たないカスタムデリータ
  );
  Keypad.addKeyEventListener(keyMapPtr);

  // イヤホン端子スイッチ
  pinMode(GPIO_NUM_18, INPUT_PULLUP);

  // 3ボタン
  pinMode(GPIO_NUM_BACK, INPUT_PULLUP);
  pinMode(GPIO_NUM_HOME, INPUT_PULLUP);
  pinMode(GPIO_NUM_MENU, INPUT_PULLUP);

  bool isHeadphone = digitalRead(GPIO_NUM_18);
  Output.Internal.begin(isHeadphone ? OutputInternal::AudioOutput::headphone : OutputInternal::AudioOutput::speaker);

  // PlayScreen UI の初期化
  playScreen.create();

  // コードが鳴ったときにコード名を表示する
  Pipeline.addChordFilter(new MainChordFilter());
  // テンポが変更されたときに表示を更新する
  Tempo.addListener(new MainTempoCallbacks());

  // 初期UI状態の更新
  update_battery();
  update_scale();
  update_tempo();
}

void loop()
{
  // M5.update()とKeypad.update()はI2Cスレッドで処理される
  
  unsigned long ms = millis();
  BtnBack.setRawState(ms, digitalRead(GPIO_NUM_BACK) == 0);
  BtnHome.setRawState(ms, digitalRead(GPIO_NUM_HOME) == 0);
  BtnMenu.setRawState(ms, digitalRead(GPIO_NUM_MENU) == 0);

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

  // ヘッドフォン抜き差し
  static bool isHeadphonePreviously = digitalRead(GPIO_NUM_18);
  bool isHeadphone = digitalRead(GPIO_NUM_18);
  if(isHeadphonePreviously != isHeadphone) {
    Output.Internal.begin(isHeadphone ? OutputInternal::AudioOutput::headphone : OutputInternal::AudioOutput::speaker);
    isHeadphonePreviously = isHeadphone;
  }

  // 仮でホームボタンを押したらバッテリー残量が表示されるようにする
  if (BtnHome.wasPressed()) update_battery();
  // if (BtnHome.wasPressed()) M5.Display.clear();

  // 仮でホームボタン長押ししたらKantanChordと切り替えるようにする
  if (BtnHome.wasHold()) {
    if (currentKeyMap == KeyMap::getAvailableKeyMaps()[0].get()) {
      currentKeyMap = KeyMap::getAvailableKeyMaps()[1].get();
    } else {
      currentKeyMap = KeyMap::getAvailableKeyMaps()[0].get();
    }

    // 一応、centerNoteNoを戻す
    *centerNoteNo = 60;
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
  if (needsTickUpdate && playScreen.isShown()) {
    playScreen.updateTick(lastTickTiming & TempoController::TICK_TIMING_BAR);
    needsTickUpdate = false;
  }
  lv_task_handler();

  while(millis() - lastLoopMillis < 5);
  lastLoopMillis = millis();
}