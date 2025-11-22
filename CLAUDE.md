# CLAUDE.md

このファイルは、Claude Code (claude.ai/code) がこのリポジトリで作業する際のガイダンスを提供します。

## プロジェクト概要

CapsuleChord2は、PlatformIOとArduinoフレームワークで構築されたM5Stackデバイス（Core2/CoreS3）向けの組み込み音楽機器プロジェクトです。コード演奏、ドラムパッド、ベース、シーケンサー、自動演奏機能など、複数の音楽アプリケーションを内蔵オーディオサンプラーで提供します。

## ビルドコマンド

### 事前準備

```bash
source ~/.platformio/penv/bin/activate
```

### ビルド

```bash
# 通常ビルド
PLATFORMIO_CORE_DIR=.pio pio run

# クリーンビルド
PLATFORMIO_CORE_DIR=.pio pio run -t clean
```

### 実機への書き込み

```bash
PLATFORMIO_CORE_DIR=.pio pio run -t upload
```

`shell` ツール実行時は `with_escalated_permissions: true` と  `justification: "Firmware upload requires unrestricted USB device access beyond sandbox permissions."` を必ず指定する。

### シリアルモニタ

- PlatformIO のシリアルモニタはターミナル環境によって ioctl エラーになる場合があるため、代わりに PlatformIO 仮想環境に入って Python の `pyserial` を使用する。
- 実行例:
  ```
  source ~/.platformio/penv/bin/activate
  python - <<'PY'
  import serial
  import time
  port = '/dev/ttyACM0'
  baud = 115200
  with serial.Serial(port, baud, timeout=0.5) as ser:
      start = time.time()
      while time.time() - start < 5:
          raw = ser.readline()
          if raw:
              print(raw.decode('utf-8', errors='replace').strip())
  PY
  ```
- `port` は `pio device list` で接続先デバイス（例: `/dev/ttyACM0`）を確認してから指定する。

## アーキテクチャ

### コアシステム

**ChordPipeline** (`ChordPipeline.h/cpp`)
- キー入力からオーディオ出力までの流れを管理する中央オーディオルーティングシステム
- `ChordFilter`と`NoteFilter`インターフェースを使ったフィルターパターンを使用
- フィルターはパイプライン内のコード/ノートイベントを監視または変更可能
- グローバルインスタンス：`Pipeline`

**Context** (`Context.h`)
- グローバル状態への共有アクセスを提供するシングルトン
- 以下へのポインタを含む：`Settings`、`Scale`、`ChordPipeline`、`CapsuleChordKeypad`
- すべてのアプリは`setContext()`でContextを受け取り、`context`メンバーでアクセス可能
- アプリ間通信用の「knock」メカニズムを`KnockListener`経由で実装

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

# その他

- `sudo` 実行はパスワード入力ができないため失敗する。必ず `with_escalated_permissions` を使うか、udev で一般ユーザーに権限を与える。

## Task Master AI Instructions
**Import Task Master's development workflow commands and guidelines, treat as if import is in the main CLAUDE.md file.**
@./.taskmaster/CLAUDE.md
