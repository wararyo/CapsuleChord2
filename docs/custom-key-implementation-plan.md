# Custom Key Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** メニュー画面上でスタブになっているカスタムキー1/2を、ユーザー設定可能な `DegreeChord` として永続化し、演奏画面で押下時に現在の調に応じたコードとして鳴らせるようにする。

**Architecture:** カスタムキー設定は `Settings.controls` に `DegreeChord` と seventh policy を含む専用構造体として保持する。メニューからは `DegreeChordInputDialog` を開き、ダイアログ表示中だけ専用 `KeyEventListener` をキーパッド入力スタックの上に積んで、通常演奏用 KeyMap より先にコード入力を受け取る。ただし `CapsuleChordKeypad::_listeners` はI2Cスレッドで走査されるため、UIスレッドから直接 `addKeyEventListener()` / `removeKeyEventListener()` せず、遅延コマンドキューを通してI2Cスレッドの安全地点で登録/解除を適用する。演奏側は現在デフォルト採用されている `KantanChordKeyMap` を優先して対応し、必要最小限で `CapsuleChordKeyMap` にも追従できる形にする。

**Tech Stack:** ESP-IDF / PlatformIO, M5Unified, LVGL 8, CapsuleChord `SettingsStore`, `Keypad::KeyEventListener`, `Chord` / `DegreeChord` / `Scale`.

---

## 背景と現状

### 現在のデフォルトKeyMap

`src/KeyMap/KeyMap.h` では `KantanChordKeyMap` が先頭に登録されており、`src/main.cpp` で `KeyMap::getAvailableKeyMaps()[0]` が使われるため、現在のデフォルトは `KantanChordKeyMap` である。

```cpp
// src/KeyMap/KeyMap.h
availableKeyMaps.push_back(std::make_shared<KantanChordKeyMap>());
availableKeyMaps.push_back(std::make_shared<CapsuleChordKeyMap>());
```

したがって、実装優先順位は次の通りとする。

1. `KantanChordKeyMap`
2. 共通化可能な入力変換ヘルパー
3. 必要に応じて `CapsuleChordKeyMap`

### 現在のカスタムキー扱い

`KantanChordKeyMap::numberKeyMap` ではコメント上は `Custom1` / `Custom2` だが、実際には通常の度数キーとして処理されている。

```cpp
const uint8_t KantanChordKeyMap::numberKeyMap[] = {
    6, //Custom1
    0, //VII
    0, //Custom2
    3, //IV
    4, //I
    5, //V
    0, //II
    1, //VI
    2};
```

`button == 1` がカスタムキー1、`button == 3` がカスタムキー2の物理位置なので、これらは通常のダイアトニック番号配列から外して明示的に処理する。

---

## 仕様

- カスタムキーは2つ。
  - カスタムキー1: 左キーパッド中央上。現行キーコードでは `KEY_LEFT_2` / button `2`。
    - **訂正 (2026-07-07):** 当初 `KEY_LEFT_1` としていたのは誤り。`KEY_LEFT_1` はダイアトニックコードの7番 (number=6) を鳴らす通常キー。
  - カスタムキー2: 左キーパッド右上。現行キーコードでは `KEY_LEFT_3` / button `3`。
- 設定項目は内部的に `DegreeChord` を保持する。
- さらに将来のリアルタイム修飾に備えて、「セブンスキーを押したときに追加する seventh の種類」を設定構造に含める。
- 初回実装では seventh policy のUIは出さなくてよい。
- カスタムキーのメニュー項目を選ぶと、`DegreeChord` の入力ダイアログが開く。
- 何らかのコードをキーパッドで打ち込むと、ダイアログが閉じて設定が反映される。
- `PlayScreen` でカスタムキーを押すと、その時点の調において設定済み `DegreeChord` が鳴る。
- 将来メニュー画面をキーパッドで操作することを考え、`ChordFilter` ではなく、専用のキーパッド入力レイヤーで受け取る。

---

## 設定データ設計

### 方針

`customKey1` / `customKey2` を単なる `DegreeChord` ではなく、将来拡張を見越した構造体として保持する。

理由:

- 将来的に「カスタムキーにもリアルタイム修飾を追加で反映する」予定がある。
- その場合、セブンスキー押下時に追加される seventh の種類を設定として持つ必要がある。
- 後から設定ファイル構造を変えると migration が必要になるため、初回から構造に含める。

### 追加する型案

`src/CustomKeySettings.h` を新設する案が分かりやすい。

```cpp
#pragma once

#include <stdint.h>
#include "Archive.h"
#include "Chord.h"

struct CustomKeyAssignment {
    enum class SeventhPolicy : uint8_t {
        DominantSeventh = 0,  // 7
        MajorSeventh = 1,     // M7
        DiatonicSeventh = 2   // 現在のスケール/度数に応じた7th
    };

    DegreeChord chord;
    SeventhPolicy seventhPolicy;

    CustomKeyAssignment()
        : chord(DegreeChord::IISharp, 0), seventhPolicy(SeventhPolicy::DominantSeventh) {}

    CustomKeyAssignment(DegreeChord chord, SeventhPolicy seventhPolicy)
        : chord(chord), seventhPolicy(seventhPolicy) {}

    bool operator==(const CustomKeyAssignment& other) const {
        return chord.root == other.chord.root &&
               chord.option == other.chord.option &&
               chord.inversion == other.chord.inversion &&
               chord.bass == other.chord.bass &&
               seventhPolicy == other.seventhPolicy;
    }

    bool operator!=(const CustomKeyAssignment& other) const {
        return !(*this == other);
    }

    void serialize(OutputArchive& archive, const char* key) const {
        archive.pushNest(key);
        archive("chord", chord);
        archive("seventhPolicy", static_cast<uint8_t>(seventhPolicy));
        archive.popNest();
    }

    void deserialize(InputArchive& archive, const char* key) {
        if (!archive.pushNest(key)) return;
        archive("chord", chord);
        uint8_t rawPolicy = static_cast<uint8_t>(seventhPolicy);
        archive("seventhPolicy", rawPolicy);
        if (rawPolicy <= static_cast<uint8_t>(SeventhPolicy::DiatonicSeventh)) {
            seventhPolicy = static_cast<SeventhPolicy>(rawPolicy);
        }
        archive.popNest();
    }
};
```

### 設定JSONイメージ

`/settings/controls.json` は以下のような形にする。

```json
{
  "version": 1,
  "customKey1": {
    "chord": {
      "Root": 3,
      "Option": 0
    },
    "seventhPolicy": 0
  },
  "customKey2": {
    "chord": {
      "Root": 10,
      "Option": 0
    },
    "seventhPolicy": 0
  },
  "leftTrigger": 0,
  "rightTrigger": 1
}
```

`Root` は `DegreeChord` の半音ベース表現を使う。例: `I = 0`, `II = 2`, `bIII = 3` (`DegreeChord::IISharp`), `IV = 5`, `V = 7`, `bVII = 10` (`DegreeChord::VISharp`)。

### migration方針

現状の `customKey1` / `customKey2` はスタブ値なので、厳密な値移行は不要とする。既存の数値値は `DegreeChord` として読めないため、読み込み時はデフォルト値にフォールバックする。

もし `InputArchive::pushNest()` がオブジェクトでない値に対して安全に false を返すなら、上記 `deserialize()` で自然にフォールバックできる。実装時には既存JSONでクラッシュしないことを native test または手動テストで確認する。

---

## コード入力設計

### ChordFilterを使わない理由

`ChordFilter` は `ChordPipeline` に流れた後の `Chord` を見るため、どの度数キーを押したか、どの修飾キーを同時押ししたかを失いやすい。カスタムキー設定では `DegreeChord` として保存したいので、キーパッドイベント段階で解釈する。

### 専用入力レイヤー

`DegreeChordInputDialog` 表示中だけ、専用の `KeyEventListener` を通常演奏用 KeyMap より上位に登録する。ただし、登録/解除をUIスレッドから直接実行してはいけない。

`CapsuleChordKeypad::_listeners` は現状ミューテックスのない `std::vector` であり、`Keypad.processKeyEvent()` はI2Cハンドラスレッド上でこれを逆順走査する。

```cpp
for (auto it = _listeners.rbegin(); it != _listeners.rend(); ++it) {
    bool consumed = listener->onKeyPressed(keyCode);
    if (consumed) return true;
}
```

一方、メニューやダイアログはメインループ/LVGL側で動く。メニュー側から直接 `Keypad.addKeyEventListener()` / `Keypad.removeKeyEventListener()` を呼ぶと、I2Cスレッドが `_listeners` を走査中に別スレッドが `push_back()` / `erase()` する可能性があり、`std::vector` の再確保やイテレータ無効化による未定義動作につながる。

したがって、この機能では **遅延コマンドキュー** を導入する。UIスレッドは「登録/解除要求」をキューに積むだけにし、実際の `_listeners` 変更は `CapsuleChordKeypad::update()` の冒頭またはイベント処理前後など、I2Cスレッド上の安全地点で適用する。

この仕組みにより、ダイアログ表示中だけ通常演奏を止めて入力完了イベントとして扱いつつ、`_listeners` のクロススレッド競合を避ける。

### 遅延コマンドキューの方針

`CapsuleChordKeypad` に listener 操作用の小さなキューを追加する。

- UIスレッド向けAPI
  - `queueAddKeyEventListener(std::shared_ptr<KeyEventListener> listener)`
  - `queueRemoveKeyEventListener(std::shared_ptr<KeyEventListener> listener)`
- I2Cスレッド側処理
  - `CapsuleChordKeypad::update()` の安全地点で `applyPendingListenerCommands()` を呼ぶ。
  - `_listeners` の `push_back()` / `erase()` はこの関数内でのみ行う。
- キュー保護
  - pending command queue は `portMUX_TYPE` または FreeRTOS queue で保護する。
  - critical section 内ではキューへのpush/popだけを行い、listener callback は呼ばない。

擬似コード:

```cpp
enum class ListenerCommandType { Add, Remove };

struct ListenerCommand {
    ListenerCommandType type;
    std::shared_ptr<KeyEventListener> listener;
};

void CapsuleChordKeypad::queueAddKeyEventListener(std::shared_ptr<KeyEventListener> listener) {
    portENTER_CRITICAL(&listenerCommandMutex);
    pendingListenerCommands.push_back({ListenerCommandType::Add, listener});
    portEXIT_CRITICAL(&listenerCommandMutex);
}

void CapsuleChordKeypad::applyPendingListenerCommands() {
    std::vector<ListenerCommand> commands;
    portENTER_CRITICAL(&listenerCommandMutex);
    commands.swap(pendingListenerCommands);
    portEXIT_CRITICAL(&listenerCommandMutex);

    for (const auto& command : commands) {
        if (command.type == ListenerCommandType::Add) {
            addKeyEventListener(command.listener);      // I2Cスレッド内でのみ実行
        } else {
            removeKeyEventListener(command.listener);   // I2Cスレッド内でのみ実行
        }
    }
}
```

`addKeyEventListener()` / `removeKeyEventListener()` は低レベルAPIとして残してもよいが、setup以降のUIコードからは直接呼ばない。名前を `addKeyEventListenerImmediate()` のように変えるか、コメントで「I2Cスレッドまたは初期化時のみ」と明記する。

### 入力レイヤーの役割

`src/Widget/DegreeChordInputDialog.h/.cpp` 内にネストクラスとして持つか、`src/KeyMap/DegreeChordInput.h/.cpp` として分離する。

初回実装では、将来のメニュー操作にも流用しやすいよう、入力変換ロジックは UI と分離するのがおすすめ。

```text
src/KeyMap/ChordKeyInput.h
src/KeyMap/ChordKeyInput.cpp
src/Widget/DegreeChordInputDialog.h
src/Widget/DegreeChordInputDialog.cpp
```

---

## ChordKeyInput 共通化案

### 目的

`KantanChordKeyMap` と `DegreeChordInputDialog` で、左キーパッド + 修飾キーから `DegreeChord` / `Chord` を作る処理を共有する。

### API案

```cpp
struct DegreeChordInputResult {
    bool valid = false;
    DegreeChord degreeChord;
};

class ChordKeyInput {
public:
    static bool isCustomKey1(uint8_t keyCode);
    static bool isCustomKey2(uint8_t keyCode);

    static DegreeChordInputResult buildDegreeChordFromKantanKey(
        uint8_t keyCode,
        const Scale& scale,
        const CapsuleChordKeypad& keypad
    );

    static Chord buildPlayableChord(
        const CustomKeyAssignment& assignment,
        const Scale& scale,
        int centerNoteNo,
        bool applyRealtimeModifiers,
        const CapsuleChordKeypad& keypad
    );
};
```

### 初回実装での注意

- `KantanChordKeyMap` の物理配置を優先する。
- `CapsuleChordKeyMap` は後続対応でもよいが、変換ヘルパーを作るなら同時に対応しやすい。
- `Keypad[KEY_RIGHT_5].isPressed()` による seventh 入力は、設定ダイアログでは `DegreeChord.option` に反映する。
- カスタムキー演奏時のリアルタイム修飾は初回では `false` とし、保存された `DegreeChord` をそのまま鳴らす。
- ただし seventh policy は設定データとして持っておく。

---

## SeventhPolicy の意味

`CustomKeyAssignment::seventhPolicy` は、将来的に `applyRealtimeModifiers == true` のとき、右手のセブンスキーを押した場合に何を足すかを決める。

候補:

- `DominantSeventh`: 常に `Chord::Seventh` を足す。
- `MajorSeventh`: 常に `Chord::MajorSeventh` を足す。
- `DiatonicSeventh`: 現在のスケールと度数に応じて `M7`, `7`, `m7`, `m7-5` などを選ぶ。

初期値はユーザー指定により `DominantSeventh` とする。将来的にリアルタイム修飾を反映する場合、セブンスキー押下時はまず `Chord::Seventh` を足す挙動から始める。

初回UIには出さないが、将来的にはカスタムキー入力ダイアログ内、または別の詳細設定で切り替えられるようにする。

---

## 実装タスク

### Task 1: 設定用型 `CustomKeyAssignment` を追加する

**Objective:** カスタムキー設定を `DegreeChord` + seventh policy で保存できる型を用意する。

**Files:**

- Create: `src/CustomKeySettings.h`
- Modify: `src/SettingsStore.h`
- Modify: `src/SettingsStore.cpp`
- Test: `test/test_music.cpp` または新規 `test/test_settings_types.cpp`

**Steps:**

1. `src/CustomKeySettings.h` を作成する。
2. `SettingsStore.h` で `#include "CustomKeySettings.h"` を追加する。
3. `ControlsSettings::customKey1/customKey2` を `SettingDescriptor<CustomKeyAssignment>` に変更する。
4. `SettingsStore.cpp` のデフォルト値を設定する。
   - customKey1: `DegreeChord(DegreeChord::IISharp, 0)` = bIII、`SeventhPolicy::DominantSeventh`
   - customKey2: `DegreeChord(DegreeChord::VISharp, 0)` = bVII、`SeventhPolicy::DominantSeventh`
5. `CustomKeyAssignment::operator==` / `operator!=` を実装する。`SettingDescriptor<T>::set()` は `value == newValue` を使うため、これが無いと `customKey1.set(...)` / `customKey2.set(...)` の時点でビルドできない。
   - `DegreeChord` は `operator==` を持たないため、`chord.root` / `chord.option` / `chord.inversion` / `chord.bass` を直接比較する。
6. 既存の `serializeItems()` / `deserializeItems()` は同じ呼び出しのままビルドできることを確認する。

**Verification:**

```bash
source ~/.platformio/penv/bin/activate
PLATFORMIO_CORE_DIR=.pio pio test -e native-test
```

---

### Task 2: KantanChordKeyMap のカスタムキー位置を通常度数配列から外す

**Objective:** `KEY_LEFT_1` / `KEY_LEFT_3` を通常の度数キーとして誤って処理しないようにする。

**Files:**

- Modify: `src/KeyMap/KantanChordKeyMap.h`
- Modify: `src/KeyMap/KantanChordKeyMap.cpp`

**Steps:**

1. `numberKeyMap` で `KEY_LEFT_1` / `KEY_LEFT_3` に対応する要素を sentinel にする。
2. `isCustomKey1()` / `isCustomKey2()` 相当の判定を追加する。
3. `onKeyPressed()` の先頭でカスタムキーを明示分岐する。
4. カスタムキー押下時は `Settings.controls.customKey1/2.get()` を取得する。
5. `Scale::degreeToChord()` で `Chord` に変換し、`centerNoteNo` で転回を計算して `Pipeline.playChord()` する。
6. `currentPressingButton` にカスタムキーの keyCode も保存し、release時に既存ロジックで止める。

**Verification:**

- 通常のI/II/IV/V/VI/VIIキーが従来通り鳴る。
- カスタムキー1/2が通常の度数キーとして扱われない。
- カスタムキー押下中だけ音が鳴り、離すと止まる。

---

### Task 3: `ChordKeyInput` ヘルパーを追加する

**Objective:** キーパッド入力から `DegreeChord` を作る処理をUI入力とKeyMapで共有できるようにする。

**Files:**

- Create: `src/KeyMap/ChordKeyInput.h`
- Create: `src/KeyMap/ChordKeyInput.cpp`
- Modify: `src/KeyMap/KantanChordKeyMap.cpp`

**Steps:**

1. Kantan配列用の物理キー → 度数番号変換をヘルパーに切り出す。
2. `buildDegreeChordFromKantanKey()` を実装する。
3. 右キーパッド修飾を `DegreeChord.option` に反映する。
4. `KantanChordKeyMap` はヘルパーから得た `DegreeChord` を `Scale::degreeToChord()` して鳴らすようにする。

**Verification:**

```bash
source ~/.platformio/penv/bin/activate
PLATFORMIO_CORE_DIR=.pio pio test -e native-test
PLATFORMIO_CORE_DIR=.pio pio run
```

---

### Task 4: Keypad listener の遅延コマンドキューを追加する

**Objective:** UIスレッドから `_listeners` を直接変更せず、I2Cスレッドの安全地点で listener 登録/解除を適用できるようにする。

**Files:**

- Modify: `src/Keypad.h`
- Modify: `src/Keypad.cpp`

**Steps:**

1. `CapsuleChordKeypad` に listener command 型を追加する。
   - `enum class ListenerCommandType { Add, Remove };`
   - `struct ListenerCommand { ListenerCommandType type; std::shared_ptr<KeyEventListener> listener; };`
2. `pendingListenerCommands` と `listenerCommandMutex` を追加する。
3. UI/メインスレッドから呼ぶためのAPIを追加する。
   - `queueAddKeyEventListener(std::shared_ptr<KeyEventListener> listener)`
   - `queueRemoveKeyEventListener(std::shared_ptr<KeyEventListener> listener)`
4. `applyPendingListenerCommands()` を追加し、pending queue をローカルにswapしてから `_listeners` に反映する。
5. `CapsuleChordKeypad::update()` のイベント読み取り/dispatch前の安全地点で `applyPendingListenerCommands()` を呼ぶ。
6. 既存の `addKeyEventListener()` / `removeKeyEventListener()` は setup時やI2Cスレッド内でのみ使う低レベルAPIとして扱う。必要ならコメントで明記する。

**Verification:**

- 既存の setup時 KeyMap 登録がこれまで通り動く。
- ダイアログ表示/破棄を繰り返してもクラッシュしない。
- キーパッド入力連打中にダイアログを開閉しても `_listeners` 走査中の vector 変更が発生しない。
- `PLATFORMIO_CORE_DIR=.pio pio run` が通る。

**Pitfalls:**

- critical section 内で listener callback を呼ばない。
- critical section 内で `_listeners` を走査しない。
- `processKeyEvent()` 実行中に同じ `_listeners` を変更しない。必ず `update()` のdispatch前後など、走査していない地点で反映する。
- queued remove は、対象listenerが未登録でも安全にno-opになるようにする。
- dialog側は `queueRemoveKeyEventListener()` を呼んだ直後でも、I2Cスレッドが次に `update()` するまで listener が一時的に残る可能性を許容する。listener側で `active` フラグを見て無効化できるとより安全。

---

### Task 5: `DegreeChordInputDialog` を追加する

**Objective:** メニューから開ける、キーパッド入力待ちのLVGLダイアログを作る。

**Files:**

- Create: `src/Widget/DegreeChordInputDialog.h`
- Create: `src/Widget/DegreeChordInputDialog.cpp`
- Modify: `src/Widget/MenuScreen.h`
- Modify: `src/Widget/MenuScreen.cpp`

**Steps:**

1. ダイアログの `create(title, currentValue, onComplete)` を実装する。
2. ダイアログ用 `KeyEventListener` を生成し、`Keypad.queueAddKeyEventListener()` で登録要求を積む。
3. 左キーパッドの有効なコード入力を受けたら `ChordKeyInput::buildDegreeChordFromKantanKey()` で `DegreeChord` を作る。
4. listener内ではLVGLを直接触らず、入力完了値と `closeRequested` / `completed` フラグだけを更新する。
5. `DegreeChordInputDialog::updateIfNeeded()` で `onComplete(degreeChord)` を呼び、ダイアログを閉じる。
6. ダイアログを閉じる際は `Keypad.queueRemoveKeyEventListener()` で解除要求を積む。
7. `del()` でも必ず解除要求を積む。ただし二重解除要求を避けるため、`listenerQueued` / `removeQueued` のような状態フラグを持つ。
8. listenerがI2Cスレッド側に一時的に残っても安全なよう、dialog破棄前に listener の `active=false` を設定する。

**Pitfalls:**

- UIスレッドから `Keypad.addKeyEventListener()` / `Keypad.removeKeyEventListener()` を直接呼ばない。必ず queue API を使う。
- ダイアログ破棄時に解除要求を積み忘れると、その後の通常演奏入力が奪われ続ける。
- `queueRemoveKeyEventListener()` 後、I2Cスレッドが次に `update()` するまで listener が残る可能性があるため、listener自身に `active` フラグを持たせて即時無効化する。
- LVGLオブジェクトの削除は現在のUIライフサイクルに合わせ、`MenuScreen::update()` 経由で安全に行う。
- `Keypad` イベントはI2C側スレッドから来るため、入力listener内で重いLVGL操作を直接行わない。フラグを立てて `DegreeChordInputDialog::updateIfNeeded()` で閉じる。

---

### Task 6: `MenuItemChord` を追加してメニューに接続する

**Objective:** `カスタムキー1` / `カスタムキー2` のメニュー項目を、選択式スタブからコード入力式に置き換える。

**Files:**

- Modify: `src/Widget/MenuItem.h`
- Modify: `src/Widget/MenuItem.cpp`
- Modify: `src/Widget/MenuScreen.h`
- Modify: `src/Widget/MenuScreen.cpp`

**Steps:**

1. `MenuItemType` に `Chord` を追加する、または専用クラスだけ追加して既存分岐を増やさない。
2. `MenuItemChord` を追加する。
3. 値表示には `CustomKeyAssignment.chord.toString()` を使う。
4. `onClick()` で `menuScreen->showDegreeChordInputDialog(this)` を呼ぶ。
5. `MenuScreen` に `DegreeChordInputDialog degreeChordInputDialog;` を持たせる。
6. `MenuScreen::update()` で `degreeChordInputDialog.updateIfNeeded()` を呼ぶ。
7. `buildControlsCategory()` の `CAPSULECHORD_SHOW_DEV_SETTINGS` 内にある既存スタブを削除し、カスタムキー項目は常時表示にする。

**Verification:**

- メニューの「操作」カテゴリがrelease/dev問わず表示される。
- カスタムキー1/2の現在値が表示される。
- 項目選択で入力ダイアログが開く。
- キーパッドでコード入力すると閉じて値が更新される。

---

### Task 7: 設定保存と読み込みを確認する

**Objective:** カスタムキー設定がLittleFS上の `/settings/controls.json` に保存され、再起動後も復元されることを確認する。

**Files:**

- Modify as needed: `src/SettingsStore.*`
- Optional Test: settings serialization test

**Steps:**

1. カスタムキー1/2をメニューから変更する。
2. 5秒以上待ち、`Settings.saveIfDirty()` の遅延保存を待つ。
3. 再起動する。
4. メニュー表示値が保持されていることを確認する。
5. カスタムキー押下時の鳴り方も保持されていることを確認する。

**Verification:**

- `/settings/controls.json` に `customKey1.chord` / `customKey1.seventhPolicy` が保存される。
- 旧形式の数値 `customKey1` が存在してもクラッシュせずデフォルト値になる。

---

### Task 8: CapsuleChordKeyMap への追従対応を検討する

**Objective:** デフォルトではないが、`CapsuleChordKeyMap` でもカスタムキーが変な通常度数として鳴らないようにする。

**Files:**

- Modify: `src/KeyMap/CapsuleChordKeyMap.cpp`
- Modify: `src/KeyMap/CapsuleChordKeyMap.h`

**Steps:**

1. `KEY_LEFT_1` / `KEY_LEFT_3` を明示的にカスタムキー扱いする。
2. 通常度数配列から sentinel で外す。
3. 可能なら `ChordKeyInput` の共通ヘルパーを使う。

**Verification:**

- `CapsuleChordKeyMap` を手動で選べる状態にした場合も、カスタムキーがIなどとして誤発火しない。

---

## 受け入れ条件

- [ ] `KantanChordKeyMap` でカスタムキー1/2が設定済み `DegreeChord` として鳴る。
- [ ] 通常のKantanキー配列の演奏挙動が維持される。
- [ ] カスタムキー設定が `/settings/controls.json` に `DegreeChord` + `seventhPolicy` として保存される。
- [ ] カスタムキー設定UIはrelease buildでも表示される。
- [ ] 入力ダイアログ表示中はキーパッド入力で設定でき、通常演奏は発火しない。
- [ ] ダイアログを閉じた後、通常演奏入力が復帰する。
- [ ] キーパッド入力中にダイアログを開閉しても、`_listeners` のクロススレッド変更が発生しない。
- [ ] 旧スタブ設定値が存在してもクラッシュしない。
- [ ] `PLATFORMIO_CORE_DIR=.pio pio test -e native-test` が通る。
- [ ] `PLATFORMIO_CORE_DIR=.pio pio run` が通る。

---

## 決定事項

1. カスタムキー1/2のデフォルト値
   - customKey1: bIII (`DegreeChord::IISharp`, root=3)
   - customKey2: bVII (`DegreeChord::VISharp`, root=10)
2. seventh policy のデフォルト
   - customKey1/customKey2 ともに `DominantSeventh`
3. 初回実装でのリアルタイム修飾
   - 将来的に反映させる前提で設定構造と変換口は用意する。
   - 現在の初回実装では、演奏時のリアルタイム修飾は無視し、保存された `DegreeChord` をそのまま鳴らす。

---

## 実装時の注意

- `SettingDescriptor<T>::get()` は参照を返すが、演奏処理では必要に応じてコピーして使う。
- `Scale::degreeToChord()` は `DegreeChord.option` をそのまま使う。ダイアトニックの m/M/7th を保存したい場合は、入力時点で `DegreeChord.option` に正しく反映する。
- UIコールバックやキーパッドlistener内で重い処理や危険なLVGL操作を直接行わない。
- UIスレッドから `CapsuleChordKeypad::_listeners` を直接変更しない。setup後の動的登録/解除は遅延コマンドキュー経由でI2Cスレッド側に適用する。
- listener登録/解除要求は必ず対にする。
- `KantanChordKeyMap::currentPressingButton` の挙動を壊さない。複数左キー押下時のNoteOff制御に使われている。
