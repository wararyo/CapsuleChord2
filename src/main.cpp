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

#define GPIO_NUM_BACK GPIO_NUM_7
#define GPIO_NUM_HOME GPIO_NUM_5
#define GPIO_NUM_MENU GPIO_NUM_8

unsigned long lastLoopMillis = 0;

m5::Button_Class BtnBack;
m5::Button_Class BtnHome;
m5::Button_Class BtnMenu;

lv_obj_t *chordlabel;
lv_style_t style_chordlabel;
lv_obj_t *battery;
lv_obj_t *scale_label;
lv_obj_t *tempo_label;
lv_obj_t *tickframe;
TempoDialog tempoDialog;

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
  // バッテリー残量を取得
  int32_t level = M5.Power.getBatteryLevel();
  bool isCharging = M5.Power.isCharging();
  lv_battery_set_level(battery, level);
  lv_battery_set_charging(battery, isCharging);
}

void update_scale() {
  lv_label_set_text(scale_label, scale->toString().c_str());
}

void update_tempo() {
  char text[64] = {'\0'};
  sprintf(text, "%d 4/4", Tempo.getTempo());
  lv_label_set_text(tempo_label, text);
}

class MainTempoCallbacks: public TempoController::TempoCallbacks {
    void onTempoChanged(TempoController::tempo_t tempo) override {
      update_tempo();
    }
    void onTick(TempoController::tick_timing_t timing) override {
      lv_tickframe_tick(tickframe, timing & TempoController::TICK_TIMING_BAR);
    }
    TempoController::tick_timing_t getTimingMask() override {
      return TempoController::TICK_TIMING_BAR | TempoController::TICK_TIMING_FULL;
    }
};

class MainChordPipelineCallbacks: public ChordPipeline::PipelineCallbacks {
    void onChordChanged(Chord chord) override {
      lv_chordlabel_set_chord(chordlabel, chord);
    }
};

void setup() {
  M5.begin();

  M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  Lvgl.begin();

  BtnHome.setHoldThresh(1000);
  BtnMenu.setHoldThresh(1000);
  Keypad.begin();

  Serial.println("Hello.");

  // Load settings
  if(!settings.load()){
    Serial.println("settings.json is not found in SD, so I'll try to create it.");
    if(!settings.save()) Serial.println("Setting file creation failed.");
  }

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
  context = Context(&settings);
  context.playChord = [](Chord chord) { Pipeline.playChord(chord); };
  context.sendNotes = [](bool isNoteOn, std::vector<uint8_t> notes, int vel) { Pipeline.sendNotes(isNoteOn, notes, vel); };
  Context::setContext(&context);

  // Keymap initialization
  currentKeyMap = KeyMap::getAvailableKeyMaps()[0].get();

  // イヤホン端子スイッチ
  pinMode(GPIO_NUM_18, INPUT_PULLUP);

  // 3ボタン
  pinMode(GPIO_NUM_BACK, INPUT_PULLUP);
  pinMode(GPIO_NUM_HOME, INPUT_PULLUP);
  pinMode(GPIO_NUM_MENU, INPUT_PULLUP);

  bool isHeadphone = digitalRead(GPIO_NUM_18);
  Output.Internal.begin(isHeadphone ? OutputInternal::AudioOutput::headphone : OutputInternal::AudioOutput::speaker);

  // LVGLウィジェットの初期化
  tickframe = lv_tickframe_create(lv_scr_act());
  lv_obj_set_size(tickframe, 240, 320);
  lv_obj_align(tickframe, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_clear_flag(tickframe, LV_OBJ_FLAG_CLICKABLE);
  chordlabel = lv_chordlabel_create(lv_scr_act());
  lv_chordlabel_set_chord(chordlabel, Chord());
  lv_style_init(&style_chordlabel);
  lv_style_set_text_font(&style_chordlabel, &genshin_32);
  lv_obj_add_style(chordlabel, &style_chordlabel, 0);
  lv_obj_center(chordlabel);
  battery = lv_battery_create(lv_scr_act());
  lv_obj_align(battery, LV_ALIGN_TOP_RIGHT, 0, 0);
  scale_label = lv_label_create(lv_scr_act());
  lv_obj_align(scale_label, LV_ALIGN_TOP_LEFT, 4, 28);
  tempo_label = lv_label_create(lv_scr_act());
  lv_obj_align(tempo_label, LV_ALIGN_TOP_RIGHT, -4, 28);
  lv_obj_add_flag(tempo_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(tempo_label, [](lv_event_t *e) {
      tempoDialog.create();
  }, LV_EVENT_CLICKED, NULL);

  // コードが鳴ったときにコード名を表示する
  Pipeline.addListener(new MainChordPipelineCallbacks());
  // テンポが変更されたときに表示を更新する
  Tempo.addListener(new MainTempoCallbacks());

  update_battery();
  update_scale();
  update_tempo();
}

void loop() {
  M5.update();
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
    if (BtnHome.wasPressed())
    {
      Output.Internal.NoteOn(60 + scale->key, 100, 0);
    }
    else if (BtnHome.wasReleased())
    {
      Output.Internal.NoteOff(60 + scale->key, 0, 0);
    }
    if (BtnMenu.wasPressed())
    {
      if(scale->key < 11) scale->key++;
      else scale->key = 0;
      update_scale();
    }

    Keypad.update();
    currentKeyMap->update();

  // ヘッドフォン抜き差し
  static bool isHeadphonePreviously = digitalRead(GPIO_NUM_18);
  bool isHeadphone = digitalRead(GPIO_NUM_18);
  if(isHeadphonePreviously != isHeadphone) {
    Output.Internal.begin(isHeadphone ? OutputInternal::AudioOutput::headphone : OutputInternal::AudioOutput::speaker);
    isHeadphonePreviously = isHeadphone;
  }

  // 仮でホームボタンを押したらバッテリー残量が表示されるようにする
  if (BtnHome.wasPressed()) update_battery();

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

  // 仮でメニューボタン長押ししたら最初のアプリを起動する
  if (BtnMenu.wasHold()) {
    App.launchApp(App.apps.front());
  }

  lv_timer_handler();
  while(millis() - lastLoopMillis < 33); // Keep 60fps
  lastLoopMillis = millis();
}