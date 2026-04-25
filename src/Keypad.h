#ifndef _KEYPAD_H_
#define _KEYPAD_H_

#undef min
#include <map>
#include <vector>
#include <memory>
#include <array>
#include <string>

#define KEYPAD_I2C_ADDR 0x09

// Legacy protocol I2C Commands (v2.x firmware)
#define CMD_GET_KEY_EVENT 0x01
#define CMD_SET_LED 0x02

// New protocol registers (v3.0+ firmware, SMBus style)
// Full map: src/protocol/registers.h on the keypad firmware
#define REG_FW_VERSION       0x00  // RO 2B [major, minor]
#define REG_LED_BRIGHT_BASE  0x70  // RW 1B per key (0x70 + kc)
#define REG_GLOBAL_BRIGHTNESS 0xC8 // WO 1B (write 0x00 to turn all LEDs off)
#define REG_KEY_EVENT        0xD0  // RO 1B FIFO pop (state<<7 | kc, 0x00 if empty)

// Keypad firmware protocol variant (detected at begin()).
// Legacy path will be removed once all deployed keypads ship with v3.0+.
enum class KeypadProtocol : uint8_t {
    Legacy = 0,
    V3 = 3
};

// LED Brightness Levels
#define LED_BRIGHT 0
#define LED_NORMAL 1
#define LED_DIM 2
#define LED_OFF 3

/*** TWI Structure ***/
// State(1bit) | Button(7bit)
// State: 0 is pressed, 1 is released
// Button: see the following

/*** Buttons ***/
// Group(3bit) | Button(4bit)

#define KEY_STATE_PRESSED 1
#define KEY_STATE_RELEASED 0

// Left keys (Group 0)
#define KEY_LEFT_1 0x01
#define KEY_LEFT_2 0x02
#define KEY_LEFT_3 0x03
#define KEY_LEFT_4 0x04
#define KEY_LEFT_5 0x05
#define KEY_LEFT_6 0x06
#define KEY_LEFT_7 0x07
#define KEY_LEFT_8 0x08
#define KEY_LEFT_9 0x09
// Right keys (Group 1)
#define KEY_RIGHT_1 0x11
#define KEY_RIGHT_2 0x12
#define KEY_RIGHT_3 0x13
#define KEY_RIGHT_4 0x14
#define KEY_RIGHT_5 0x15
#define KEY_RIGHT_6 0x16
#define KEY_RIGHT_7 0x17
#define KEY_RIGHT_8 0x18
#define KEY_RIGHT_9 0x19
// Other (Group 2)
#define KEY_L      0x21
#define KEY_R      0x22
#define KEY_LT     0x23
#define KEY_RT     0x24

// Key Groups
#define KEY_GROUP_LEFT  0
#define KEY_GROUP_RIGHT 1
#define KEY_GROUP_OTHER 2

// 型安全なキーイベントクラス
class KeyEvent {
public:
    // キー状態
    enum class State : uint8_t {
        Pressed = KEY_STATE_PRESSED,
        Released = KEY_STATE_RELEASED
    };

    // キーグループ
    enum class Group : uint8_t {
        Left = KEY_GROUP_LEFT,
        Right = KEY_GROUP_RIGHT,
        Other = KEY_GROUP_OTHER
    };

private:
    union {
        char rawValue;
        struct {
            uint8_t button : 4;  // ボタン番号 (0-15)
            uint8_t group : 3;   // グループID (0-7)
            uint8_t state : 1;   // 状態 (0: released, 1: pressed)
        } bits;
    } data;

public:
    // デフォルトコンストラクタ
    KeyEvent() : data{0} {}

    // 生のchar値からのコンストラクタ（後方互換性用）
    KeyEvent(char rawValue) : data{rawValue} {}

    // 各要素からのコンストラクタ
    KeyEvent(State state, Group group, uint8_t button) {
        data.bits.state = static_cast<uint8_t>(state);
        data.bits.group = static_cast<uint8_t>(group);
        data.bits.button = button;
    }

    // キーコードからのコンストラクタ
    KeyEvent(State state, uint8_t keyCode) {
        data.bits.state = static_cast<uint8_t>(state);
        data.bits.group = (keyCode >> 4) & 0x07;
        data.bits.button = keyCode & 0x0F;
    }

    // 個別のアクセサメソッド
    State getState() const { return static_cast<State>(data.bits.state); }
    Group getGroup() const { return static_cast<Group>(data.bits.group); }
    uint8_t getButton() const { return data.bits.button; }
    
    // キーコード全体を取得（グループ+ボタン）
    uint8_t getKeyCode() const { return ((data.bits.group & 0x07) << 4) | (data.bits.button & 0x0F); }
    
    // charへの変換演算子（後方互換性用）
    operator char() const { return data.rawValue; }
    
    // ボタンが押されているかどうか
    bool isPressed() const { return getState() == State::Pressed; }
    
    // イベントが特定のキーに関するものかどうかを判定
    bool isKey(uint8_t keyCode) const { return getKeyCode() == keyCode; }
};

// LED Layer class for managing LED states
class LedLayer {
public:
    // Constructor with optional name for debugging
    LedLayer(const std::string& name = "Unknown") : layerName(name) {
        // Initialize all LEDs to OFF by default
        ledStates.fill(LED_OFF);
    }

    virtual ~LedLayer() = default;

    // Set LED brightness for a specific key
    void setLed(uint8_t keyCode, uint8_t brightness) {
        if (keyCode < ledStates.size()) {
            // Ensure brightness is within valid range
            if (brightness > LED_OFF) {
                brightness = LED_OFF;
            }
            ledStates[keyCode] = brightness;
        }
    }

    // Get LED brightness for a specific key
    uint8_t getLed(uint8_t keyCode) const {
        if (keyCode < ledStates.size()) {
            return ledStates[keyCode];
        }
        return LED_OFF;
    }

    // Set multiple LEDs at once
    void setLeds(const std::map<uint8_t, uint8_t>& leds) {
        for (const auto& led : leds) {
            setLed(led.first, led.second);
        }
    }

    // Fill all LEDs (set to specified brightness, default is OFF)
    void fillLeds(uint8_t brightness = LED_OFF) {
        ledStates.fill(brightness);
    }

    // Get layer name for debugging
    const std::string& getName() const { return layerName; }

    // Get all LED states
    const std::array<uint8_t, 256>& getAllLeds() const { return ledStates; }

protected:
    std::array<uint8_t, 256> ledStates;  // Support all possible key codes (0-255)
    std::string layerName;
};

class CapsuleChordKeypad {
public: 
    class Key;

    // Key Event Listener Interface
    class KeyEventListener {
    public:
        virtual ~KeyEventListener() = default;
        
        // Called when a key is pressed
        // Return true if the event is consumed, false to pass to the next listener
        virtual bool onKeyPressed(uint8_t keyCode) = 0;
        
        // Called when a key is released
        // Return true if the event is consumed, false to pass to the next listener
        virtual bool onKeyReleased(uint8_t keyCode) = 0;
    };

private:
    std::map<int,Key> keys;
    std::vector<std::shared_ptr<KeyEventListener>> _listeners;

    // Set LED brightness for a specific key
    void setLedBrightness(uint8_t keyCode, uint8_t brightness);
    // Update all LEDs based on the current top layer
    void updateLeds();
    // Get the current top LED layer (for debugging)
    std::shared_ptr<LedLayer> getTopLedLayer() const;

    // Protocol detection + per-protocol I2C helpers
    KeypadProtocol detectProtocol();
    uint8_t readKeyEventLegacy();
    uint8_t readKeyEventV3();
    void writeLedBrightnessLegacy(uint8_t keyCode, uint8_t brightness);
    void writeLedBrightnessV3(uint8_t keyCode, uint8_t brightness);

    // LED Layer Management
    std::vector<std::shared_ptr<LedLayer>> _ledLayers;
    bool _needsLedUpdate = true;
    bool _initialized = false;  // 初期化済みフラグ
    KeypadProtocol _protocol = KeypadProtocol::Legacy;

public:
    void begin();
    void update();
    
    // Add a key event listener to the top of the stack
    void addKeyEventListener(std::shared_ptr<KeyEventListener> listener);
    
    // Remove a key event listener
    void removeKeyEventListener(std::shared_ptr<KeyEventListener> listener);
    
    // Process a key event through the listener stack
    bool processKeyEvent(const KeyEvent& event);
    
    // LED Layer Management
    // Push a new LED layer to the top of the stack
    void pushLedLayer(std::shared_ptr<LedLayer> layer);
    
    // Remove a specific LED layer from anywhere in the stack
    void removeLedLayer(std::shared_ptr<LedLayer> layer);

    void markLedNeedsUpdate();

    // Force-turn off all LEDs via REG_GLOBAL_BRIGHTNESS (V3 protocol only).
    // 電源OFF直前に呼ぶ。USB給電時、本体3.3Vが切れた後も5V系のLEDは
    // 給電が続くため、ハードウェア側で消灯させておく必要がある。
    // Legacy FWではno-op。
    void turnOffAllLeds();
    
    class Key {
        private:
            int id;
            bool mIsPressed = false;
        public:
            void press();
            void release();
            Key(){}
            Key(int keyId): id(keyId) {}
            bool isPressed();
    };

    Key operator [](int key){
        return keys[key];
    }
};

extern CapsuleChordKeypad Keypad;

#endif