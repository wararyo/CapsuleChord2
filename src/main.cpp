#include <M5Unified.h>
#include <vector>
#include <Preferences.h>
#include "BLEMidi.h"
#include "Chord.h"
#include "Scale.h"
#include "Keypad.h"
#include "KeyMap/KeyMap.h"
#include "Context.h"
#include "Output/MidiOutput.h"
#include "Tempo.h"

#define DEVICE_NAME "CapsuleChord 2"

#define GPIO_NUM_BACK GPIO_NUM_7
#define GPIO_NUM_HOME GPIO_NUM_5
#define GPIO_NUM_MENU GPIO_NUM_8

std::vector<uint8_t> playingNotes;
bool seventh = false;
int option = 0;

float expression = 64;

unsigned long lastLoopMillis = 0;

m5::Button_Class BtnBack;
m5::Button_Class BtnHome;
m5::Button_Class BtnMenu;

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

enum Scene : uint8_t {
  Connection,
  Play,
  FunctionMenu,
  length
};

Scene currentScene = Scene::length; //Specify a value other than Connection in order to draw correctly at the first changeScene()
Scene requiredToChangeScene;

void changeScene(Scene scene) {
  //To draw at the end of loop(), set flag
  requiredToChangeScene = scene;
}

//Used only in setup() and the end of loop() Do not touch it anywhere else!
void _changeScene_raw() {
  M5.Lcd.clear();

  //終了処理
  switch(currentScene) {
  }

  //開始処理
  switch(requiredToChangeScene) {
    case Scene::Connection:
      M5.Lcd.setTextFont(1);
      if(SD.exists("/capsulechord/splash.jpg")){
        // M5.Display.drawJpgFile(SD, "/capsulechord/splash.jpg", 0, 0);
      }
      else {
        M5.Lcd.setCursor(0, 48);
        M5.Lcd.setTextSize(4);
        M5.Lcd.println("CapsuleChord2");
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("BLE MIDI Chordpad Device\n");
        M5.Lcd.setTextSize(1);
        M5.Lcd.println("Advertising...");
        M5.Lcd.println("Press the button A to power off.");
      }
    break;
    case Scene::Play:
      M5.Lcd.setCursor(0,0);
      M5.Lcd.setTextSize(2);
      M5.Lcd.println(scale->toString());
    break;
    case Scene::FunctionMenu:
    break;
  }
  currentScene = requiredToChangeScene;
}

void sendNotes(bool isNoteOn, std::vector<uint8_t> notes, int vel) {
  if(isNoteOn) {
    for(uint8_t n : notes) {
      // Midi.sendNote(0x90, n, vel);
      Output.Internal.NoteOn(n, vel, 0);
    }
    playingNotes.insert(playingNotes.end(),notes.begin(),notes.end());
  }
  else {
    for(uint8_t n : playingNotes) {
      // Midi.sendNote(0x80, n, 0);
      Output.Internal.NoteOff(n, 0, 0);
    }
    playingNotes.clear();
  }
}

void playChord(Chord chord) {
  sendNotes(true,chord.toMidiNoteNumbers(),120);
  M5.Lcd.setTextSize(4);
  M5.Lcd.fillRect(0,130,240,120,BLACK);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.setTextSize(5);
  M5.Lcd.drawString(chord.toString(), 120, 160, 2);
  M5.Lcd.setTextDatum(TL_DATUM);
}

class ServerCallbacks: public BLEMidiServerCallbacks {
    void onConnect(BLEServer* pServer) {
      BLEMidiServerCallbacks::onConnect(pServer);
      changeScene(Scene::Play);
    };

    void onDisconnect(BLEServer* pServer) {
      BLEMidiServerCallbacks::onDisconnect(pServer);
      changeScene(Scene::Connection);
    }
};

class MainTempoCallbacks: public TempoController::TempoCallbacks {
    void onTempoChanged(TempoController::tempo_t tempo) override {
      Serial.printf("Tempo: %d\n", tempo);
    }
    void onTick(TempoController::tick_timing_t beat) override {
      Serial.printf("Tick: %4x\n", beat);
    }
    TempoController::tick_timing_t getTimingMask() override {
      return TempoController::TICK_TIMING_FULL | TempoController::TICK_TIMING_FULL_TRIPLET;
    }
};

void setup() {
  M5.begin();
  M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  BtnHome.setHoldThresh(1000);
  
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
  context.playChord = playChord;
  context.sendNotes = sendNotes;
  Context::setContext(&context);

  // Keymap initialization
  currentKeyMap = KeyMap::getAvailableKeyMaps()[0].get();

  // Scene initialization
  changeScene(Scene::Play);
  _changeScene_raw();

  // イヤホン端子スイッチ
  pinMode(GPIO_NUM_18, INPUT_PULLUP);

  // 3ボタン
  pinMode(GPIO_NUM_BACK, INPUT_PULLUP);
  pinMode(GPIO_NUM_HOME, INPUT_PULLUP);
  pinMode(GPIO_NUM_MENU, INPUT_PULLUP);

  // Midi.begin(DEVICE_NAME, new ServerCallbacks(), NULL);
  bool isHeadphone = digitalRead(GPIO_NUM_18);
  Output.Internal.begin(isHeadphone ? OutputInternal::AudioOutput::headphone : OutputInternal::AudioOutput::speaker);

  Tempo.setTempo(60);
  Tempo.addListener(new MainTempoCallbacks());
  Tempo.start();
}

void loop() {
  M5.update();
  unsigned long ms = millis();
  BtnBack.setRawState(ms, digitalRead(GPIO_NUM_BACK) == 0);
  BtnHome.setRawState(ms, digitalRead(GPIO_NUM_HOME) == 0);
  BtnMenu.setRawState(ms, digitalRead(GPIO_NUM_MENU) == 0);

  switch(currentScene) {
    case Scene::Connection:
      if(M5.BtnA.wasPressed()) M5.Power.deepSleep();
    break;
    case Scene::Play:
      if (BtnBack.wasPressed())
      {
        if(scale->key > 0) scale->key--;
        else scale->key = 11;
        M5.Lcd.clear();
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println(scale->toString());
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
        M5.Lcd.clear();
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println(scale->toString());
      }

      Keypad.update();
      currentKeyMap->update();
    break;
    case Scene::FunctionMenu:

    break;
  }

  // ヘッドフォン抜き差し
  static bool isHeadphonePreviously = digitalRead(GPIO_NUM_18);
  bool isHeadphone = digitalRead(GPIO_NUM_18);
  if(isHeadphonePreviously != isHeadphone) {
    Output.Internal.begin(isHeadphone ? OutputInternal::AudioOutput::headphone : OutputInternal::AudioOutput::speaker);
    isHeadphonePreviously = isHeadphone;
  }

  // 仮でホームボタンを押したらバッテリー残量が表示されるようにする
  if (BtnHome.wasPressed()) {
    // バッテリー残量を取得
    int32_t level = M5.Power.getBatteryLevel();
    char str[16] = {'\0'};
    sprintf(str, "%d%%", level);
    Serial.print(str);
    M5.Lcd.fillRect(0,300,240,20,BLACK);
    M5.Lcd.setTextDatum(BR_DATUM);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(str, 240, 320, 1);
    M5.Lcd.setTextDatum(TL_DATUM);
  }

  // 仮でホームボタン長押ししたらKantanChordと切り替えるようにする
  if (BtnHome.wasHold()) {
    M5.Lcd.fillRect(0,300,240,20,BLACK);
    M5.Lcd.setTextDatum(BL_DATUM);
    M5.Lcd.setTextSize(2);
    if (currentKeyMap == KeyMap::getAvailableKeyMaps()[0].get()) {
      currentKeyMap = KeyMap::getAvailableKeyMaps()[1].get();
      M5.Lcd.drawString("KeyMap: Kantan", 0, 320, 1);
    } else {
      currentKeyMap = KeyMap::getAvailableKeyMaps()[0].get();
      M5.Lcd.drawString("KeyMap: Capsule", 0, 320, 1);
    }
    M5.Lcd.setTextDatum(TL_DATUM);

    // 一応、centerNoteNoを戻す
    *centerNoteNo = 60;
  }

  if(currentScene != requiredToChangeScene) _changeScene_raw();
  while(millis() - lastLoopMillis < 33); // Keep 60fps
  lastLoopMillis = millis();
}