# CLAUDE.md

このファイルは、Claude Code (claude.ai/code) がこのリポジトリで作業する際のガイダンスを提供します。

## プロジェクト概要

CapsuleChord2は、PlatformIOとArduinoフレームワークで構築されたM5Stackデバイス（Core2/CoreS3）向けの組み込み音楽機器プロジェクトです。コード演奏、ドラムパッド、ベース、シーケンサー、自動演奏機能など、複数の音楽アプリケーションを内蔵オーディオサンプラーで提供します。

## ビルドコマンド

### 事前準備

PlatformIO Core が未インストールの場合は、公式 installer script でユーザー権限のままインストールする:

```bash
mkdir -p platformio-install
cd platformio-install
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

PlatformIO Core を installer script で入れた場合は、以降のコマンドで仮想環境を有効化する:

```bash
# Linux/macOS
source ~/.platformio/penv/bin/activate
pio --version
```


### ビルド

#### Linux/macOS

```bash
# 通常ビルド
PLATFORMIO_CORE_DIR=.pio pio run

# クリーンビルド
PLATFORMIO_CORE_DIR=.pio pio run -t clean
```

#### Windows

```bash
# 通常ビルド
pio run

# クリーンビルド
pio run -t clean
```

### 実機への書き込み

#### Linux/macOS

```bash
PLATFORMIO_CORE_DIR=.pio pio run -t upload
```

CLI から実行する場合は、必要に応じて PlatformIO Core の仮想環境を有効化してから実行する:

```bash
source ~/.platformio/penv/bin/activate
PLATFORMIO_CORE_DIR=.pio pio run -t upload
```

`upload_protocol = esp-builtin` は ESP USB-JTAG 経由の書き込みを行う。通常起動後に `CapsuleChord2 CDC` (`VID:PID=16C0:05E4`) として見えている時は、アプリ側のUSB MIDI/CDCコンポジットデバイスとして接続されており、JTAG書き込みはできない。この場合は `Error: esp_usb_jtag: could not find or open device!` で失敗する。

書き込み時は、ユーザーに **ホームボタンを押しながらCapsuleChordの電源を入れる** よう依頼する。これによりUSB MIDI初期化がスキップされ、`USB JTAG/serial debug unit` (`VID:PID=303A:1001`) として認識される。この状態で `PLATFORMIO_CORE_DIR=.pio pio run -t upload` を実行すると書き込みできる。書き込み後は通常の `CapsuleChord2 CDC` として再認識される。

Linux環境では `Error: libusb_open() failed with LIBUSB_ERROR_ACCESS` というエラーが出ることがある。その際は USB デバイスへのアクセス権を確認し、99-platformio-udev.rules を設定した後、デバイスを再度接続する。
また、brlttyのアンインストールが必要な場合もある。

```bash
# udev rulesの設定
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo adduser $USER dialout
sudo service udev restart

# brlttyのアンインストール
sudo apt remove brltty
```

#### Windows

```bash
pio run -t upload
```

### テスト

```bash
# PC上でテスト
PLATFORMIO_CORE_DIR=.pio pio test -e native-test
```

### シリアルモニタ

CapsuleChord 2本体はUSB CDCではなく、基板上のUARTへログを出す。PicoProbe/DebugProbeを接続している場合は、CapsuleChord本体のUSB CDC/JTAGポートではなく **DebugProbe側のCDC-ACM UARTポート** をモニタする。

接続例:

```text
PC <-- USB --> CapsuleChord 2 (ESP32-S3) <-- UART --> PicoProbe/DebugProbe <-- USB --> PC
```

Linuxではまずポートを確認する:

```bash
source ~/.platformio/penv/bin/activate
pio device list
ls -l /dev/serial/by-id/
```

例:

```text
/dev/ttyACM0: Debugprobe on QT2040 (CMSIS-DAP - CDC-ACM UART Interface)
/dev/ttyACM1: USB JTAG/serial debug unit
```

この場合、ログを見るべきポートは DebugProbe 側の CDC-ACM UART Interface（例: `/dev/ttyACM0` または `/dev/serial/by-id/usb-Raspberry_Pi_Debugprobe_on_QT2040__CMSIS-DAP_...-if01`）。`USB JTAG/serial debug unit` 側や `CapsuleChord2 CDC` 側では通常のアプリログは見えない。

Espressif IDF Monitorを使用する場合は、引数としてELFファイルのパスを指定する。

#### Windows (PowerShell)

ESP-IDF環境を有効化したPowerShellを起動し、シリアルモニタを実行する。

```powershell
# ESP-IDF環境を有効化したPowerShellを起動
C:\WINDOWS\System32\WindowsPowerShell\v1.0\powershell.exe -NoExit -ExecutionPolicy Bypass -NoProfile -Command "& {. 'C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1'}"

# シリアルモニタを起動
python -m esp_idf_monitor -- .pio\build\m5stack-cores3\firmware.elf
```

#### Linux/macOS

ESP-IDFを手動セットアップ済みの場合:

```bash
# 仮想環境を有効化
. $HOME/esp/esp-idf/export.sh

# シリアルモニタを起動
python -m esp_idf_monitor -- .pio/build/m5stack-cores3/firmware.elf
```

`$HOME/esp/esp-idf/export.sh` が無い場合は、PlatformIO が作成した ESP-IDF Python venv を使うこともできる。ポートはDebugProbe側を指定する:

```bash
source ~/.platformio/penv/bin/activate
PLATFORMIO_CORE_DIR=.pio pio run  # firmware.elf が無い場合のみ
PORT=/dev/serial/by-id/usb-Raspberry_Pi_Debugprobe_on_QT2040__CMSIS-DAP_...-if01
.pio/penv/.espidf-*/bin/python -m esp_idf_monitor -p "$PORT" -b 115200 --no-reset -- .pio/build/m5stack-cores3/firmware.elf
```

モニタ実行環境によっては TTY が必要になるため、その場合はターミナル上で直接実行するか、疑似TTYを割り当てて実行する。DebugProbe側でUARTログが読めていれば、次のようなログが表示される:

```text
W (...) I2CHandler: I2C loop took 17 ms (warning threshold: 10ms)
```

#### 代替手段

起動できなければPlatformIOのシリアルモニタで代用する。

```bash
# 仮想環境を有効化（Linux/macOS）
source ~/.platformio/penv/bin/activate

# シリアルモニタを起動（ポートは環境に合わせて変更）
PLATFORMIO_CORE_DIR=.pio pio device monitor -p /dev/ttyACM0 -b 115200
```

### LittleFSへの書き込み

`data` フォルダ以下にはLittleFSで使用するファイル群が格納されており、下記のコマンドを用いて書き込みを行う。

```bash
PLATFORMIO_CORE_DIR=.pio pio run --target uploadfs
```

## アーキテクチャ

### コアシステム

**ChordPipeline** (`ChordPipeline.h/cpp`)
- キー入力からオーディオ出力までの流れを管理する中央オーディオルーティングシステム
- `ChordFilter`と`NoteFilter`インターフェースを使ったフィルターパターンを使用
- フィルターはパイプライン内のコード/ノートイベントを監視または変更可能
- グローバルインスタンス：`Pipeline`

**TempoController** (`Tempo.h/cpp`)
- テンポ、音楽的タイミング、ティック通知を管理
- 複数のティック解像度を提供（小節、全拍、半拍、四分拍、三連符）
- リスナーは`TempoCallbacks`を実装し、`Tempo.addListener()`で登録
- グローバルインスタンス：`Tempo`

**OutputInternal** (`Output/OutputInternal.h/cpp`)
- CapsuleSamplerライブラリを使用した内蔵音源シンセシス
- I2Sオーディオ出力を管理（スピーカー/ヘッドフォン切り替え）
- 音色を格納：ピアノ、アコースティックギター、ベース、エレピ、スーパーソー、ドラムセット
- 専用FreeRTOSタスクでオーディオ処理を実行
- グローバルインスタンス：`Output.Internal`

### アプリケーションフレームワーク

**AppBase** (`App/AppBase.h`)
- すべてのアプリケーションの基底クラス
- ライフサイクル：`onCreate()` → `onActivate()` → `onShowGui()` → `onHideGui()` → `onDeactivate()` → `onDestroy()`
- アプリは`runsInBackground()`でバックグラウンド実行可能（非表示時もTempo/Pipelineにフック）
- `onUpdateGui()`でコールバックからの安全なUI更新（コールバック内でフラグ設定、メインループでUI更新）

**AppManager** (`App/AppManager.h/cpp`)
- アプリのライフサイクルと切り替えを管理
- アプリ間通信用の「knock」メカニズムを`KnockListener`インターフェースで実装
- グローバルインスタンス：`App`

`src/App/`内の利用可能なアプリ：
- `AppAutoPlay`：プログラムされた楽曲をコード変更とドラムパターンで再生
- `AppBall`：物理ベースのノートトリガー
- `AppBass`：ベースラインジェネレーター
- `AppDrumPad`：ドラムパッドインターフェース
- `AppDrumPattern`：ドラムパターンシーケンサー
- `AppMetronome`：ビジュアルティックフィードバック付きメトロノーム
- `AppSequencer`：ステップシーケンサー
- `AppSoundTest`：オーディオテストユーティリティ

### UIシステム

LVGL 8.3.4上に構築され、`LvglWrapper.h/cpp`でラップ。

`src/Widget/`内のカスタムウィジェット：
- `AppLauncher`：アプリ選択グリッド
- `PlayScreen`：コード/スケール/テンポ表示付きメイン演奏画面
- `TempoDialog`：テンポ調整ダイアログ
- カスタムLVGLウィジェット：`lv_chordlabel`、`lv_battery`、`lv_tickframe`、`lv_appbutton`

### 入力システム

**CapsuleChordKeypad** (`Keypad.h/cpp`)
- I2C経由でキーパッド入力を管理
- イベントリスナーは`KeyEventListener`インターフェースを実装
- グローバルインスタンス：`Keypad`

**KeyMap**システム (`src/KeyMap/`)
- キー押下を音楽イベントに変換
- `KeyMapBase`：基底インターフェース
- 実装：`CapsuleChordKeyMap`、`KantanChordKeyMap`

### 音楽的概念

**Chord** (`Chord.h/cpp`)
- ルート音とクオリティフラグでコードを表現
- メジャー、マイナー、ディミニッシュ、オーギュメント、各種セブンス、ナインスなどをサポート

**Scale** (`Scale.h/cpp`)
- キーとモードで音階を表現
- ノートをスケールディグリーに制約するために使用

**DegreeChord** - 絶対音ではなくスケールディグリーで表現されたコード

### スレッディングモデル

- メインループはコア1で実行
- I2C操作（M5.update()、Keypad.update()）は`I2CHandler`経由で別スレッドで実行
- オーディオ処理は専用FreeRTOSタスクで実行
- TempoコールバックはFreeRTOSタイマーから実行
- **重要**：コールバック内でブロッキング操作を呼び出さないこと。フラグと`onUpdateGui()`パターンを使用

### 初期化順序

`setup()`での初期化は以下の順序で行われ、各コンポーネントは前のステップが完了していることを前提とする：

1. **M5.begin()** - M5Stackハードウェアの初期化
2. **Lvgl.begin()** - LVGLの初期化
3. **Keypad.begin()** - キーパッドI2C通信の初期化
4. **I2C.begin()** - I2Cハンドラスレッドの開始
5. **Settings読み込み** - `scale`および`centerNoteNo`ポインタの設定
6. **KeyMap初期化** - KeyMapはこの時点で`scale`/`centerNoteNo`が有効であることを期待
7. **Output.Internal.begin()** - オーディオ出力の初期化
8. **UI作成** - PlayScreen、フィルター登録、テンポリスナー登録

**スレッドセーフティに関する注意**：
- グローバル変数（`Pipeline`, `Keypad`, `Tempo`, `App`）はコンストラクタで初期化され、`setup()`開始前に利用可能
- `scale`と`centerNoteNo`は`setup()`中に設定される。KeyMapの`onKeyPressed()`はこれらがnullの場合は早期リターンする
- FreeRTOSタイマーやI2Cスレッドからのコールバックは`setup()`完了後にのみ発生する

## MIDI変換ツール

`tools/midi_to_capsule_chord.py` - MIDIファイルをCapsuleChord2の楽曲データ形式に変換します。

### セットアップ
```bash
cd tools
python3 -m venv venv
source venv/bin/activate  # macOS/Linux
pip install -r requirements.txt
```

### 使用方法
```bash
python midi_to_capsule_chord.py --input song.mid --song_name MySong --output_dir ../src/Assets
```

### 要件
- チャンネル1：コードのルート音（現在はすべてMajorとして解釈）
- チャンネル10：ドラムパート（General MIDIドラムマップ）

### 統合
`src/Assets/`に`.h`と`.cpp`ファイルを生成した後：

1. `AppAutoPlay.cpp`にヘッダーをインクルード：
   ```cpp
   #include "../Assets/MySong.h"
   ```

2. `initializeSongs()`に追加：
   ```cpp
   availableSongs.push_back({
       "Song Name",
       MYSONG_COMMANDS,
       MYSONG_COMMAND_COUNT,
       MYSONG_TEMPO,
       MYSONG_DURATION
   });
   ```

## ハードウェア構成

対象ボード：
- M5Stack Core2 (ESP32)
- M5Stack CoreS3 (ESP32-S3) - デフォルト、デバッグサポート付き

オーディオ出力：
- I2Sスピーカーとヘッドフォンジャック（自動切り替え）
- GPIO18でヘッドフォン検出

物理ボタン：
- GPIO7：Backボタン（スケールキー減少）
- GPIO5：Homeボタン（アプリランチャー、長押しでキーマップ切り替え）
- GPIO8：Menuボタン（スケールキー増加）

## 依存関係

主要ライブラリ（`platformio.ini`参照）：
- `m5stack/M5Unified`：ハードウェア抽象化
- `lvgl/lvgl @ ^8.3.4`：UIフレームワーク
- `ArduinoJson`：設定用JSON解析
- `CapsuleSampler`：オーディオシンセシス（GitHubから）

## コードスタイルメモ

- C++17を使用（`-std=gnu++17`）
- サンプル/ティンバー管理にshared_pointerを使用
- スレッドセーフティにFreeRTOSプリミティブ（`portENTER_CRITICAL`/`portEXIT_CRITICAL`）を使用
- グローバルシステムにシングルトンパターン（Tempo、Pipeline、Keypad、Output）

## エージェント指針

- コード修正を行った場合は、上記手順に従いビルドを行い、その結果を報告する。
- 書き込み後はユーザーへ書き込み操作と動作確認の案内を行う。

## Task Master AI Instructions
**Import Task Master's development workflow commands and guidelines, treat as if import is in the main CLAUDE.md file.**
@./.taskmaster/CLAUDE.md
