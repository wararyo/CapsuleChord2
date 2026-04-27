# CapsuleChord 2

CapsuleChord 2 は、M5Stack CoreS3 上で動作する電子楽器です。
コード演奏、ドラムパッド、ベース、ステップシーケンサー、自動演奏など複数の音楽アプリを 1 台に内蔵し、
本体スピーカーまたはヘッドフォンから音を鳴らすことができます。音色合成にはオリジナルの
[CapsuleSampler](https://github.com/wararyo/CapsuleSampler) を使用しています。

## 特長

- コード/スケールに沿った演奏ができるキーパッド入力
- ピアノ、アコースティックギター、ベース、エレピ、スーパーソー、ドラムキットの内蔵音色
- LVGL ベースの GUI（コード表示、テンポ表示、アプリランチャー など）
- アプリを切り替えて使える構成（コード演奏、ドラムパッド、シーケンサー、自動演奏 など）
- スピーカー / ヘッドフォンの自動切り替え（ヘッドフォン挿入を検出）
- USB MIDIを使用したPCやスマートフォンとの接続機能

## 対応ハードウェア

- M5Stack CoreS3（ESP32-S3）

物理ボタンの割り当て:

| GPIO | 役割 |
| ---- | ---- |
| GPIO5 | Home（アプリランチャー、長押しでキーマップ切替） |
| GPIO7 | Back（スケールキー −） |
| GPIO8 | Menu（スケールキー ＋） |
| GPIO18 | ヘッドフォン検出 |

## 必要なもの

- [Visual Studio Code](https://code.visualstudio.com/)
- VSCode 拡張機能 [**PlatformIO IDE**](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
- M5Stack CoreS3 本体と USB-C ケーブル
- Git（サブモジュール/関連ライブラリの取得用）

PlatformIO は VSCode 拡張機能としてインストールするだけで、ツールチェインや ESP32 用 SDK は
自動的にダウンロード・セットアップされます。Arduino IDE や ESP-IDF を別途インストールする必要はありません。

> **補足**: 本プロジェクトは Arduino フレームワークではなく **ESP-IDF フレームワーク** を使用しています
> （`platformio.ini` の `framework = espidf`）。PlatformIO がこの差異を吸収するため、利用者側で
> 特別な設定をする必要はありません。

## セットアップ手順（VSCode + PlatformIO）

### 1. リポジトリの取得

```bash
git clone https://github.com/wararyo/CapsuleChord2.git
```

依存ライブラリ（CapsuleSampler を含む）は PlatformIO が `platformio.ini` に基づいて
自動的に取得するため、追加の clone は不要です。

### 2. VSCode で開く

`CapsuleChord2` フォルダを VSCode で開きます。
PlatformIO IDE 拡張機能がインストール済みであれば、初回起動時にツールチェインや
依存ライブラリの取得が自動的に始まります（数分かかる場合があります）。

### 3. ビルドターゲット（環境）の選択

VSCode 下部のステータスバーから環境を選択できます。通常は次のいずれかを選びます。

- `env:m5stack-cores3` — CoreS3 用（デバッグビルド、デフォルト）
- `env:m5stack-cores3-release` — CoreS3 用（リリースビルド）
- `env:native-test` — PC 上で動かすユニットテスト用

### 4. ビルド・書き込み

PlatformIO 拡張機能のサイドバー（アリのアイコン）から、または下部ステータスバーのアイコンから操作します。

| やりたいこと | サイドバー上の操作 | ステータスバーのアイコン |
| ------------ | ------------------ | ------------------------ |
| ビルド | `PROJECT TASKS` → 環境 → `General` → **Build** | チェックマーク（✓） |
| 本体への書き込み | `PROJECT TASKS` → 環境 → `General` → **Upload** | 右矢印（→） |
| クリーン | `PROJECT TASKS` → 環境 → `General` → **Clean** | ゴミ箱 |
| シリアルモニタ | `PROJECT TASKS` → 環境 → `General` → **Monitor** | コンセント |

M5Stack を USB-C ケーブルで PC に接続し、**Upload** を実行するとファームウェアが書き込まれます。

### 5. LittleFS（音色データ）の書き込み

`data/` フォルダ以下にはサンプリング音色のデータが入っており、これらは LittleFS という
ファイルシステムとして本体フラッシュに書き込みます。**初回は必ず実行してください**
（書き込まないと音が鳴りません）。

- サイドバー → `PROJECT TASKS` → 環境 → `Platform` → **Build Filesystem Image**
- 続けて → **Upload Filesystem Image**

ファームウェアと音色データは独立して書き込まれるため、ソースコードを変更しただけのときは
通常の **Upload** だけで十分です。`data/` 以下を更新したときに再度 **Upload Filesystem Image** を実行します。

### 6. テスト（任意）

PC 上で動くユニットテストが用意されています。

- `PROJECT TASKS` → `env:native-test` → **Test**

## 依存パッケージ

`platformio.ini` で管理されています。PlatformIO が自動的に取得するため、手動インストールは不要です。

| ライブラリ | 用途 |
| ---------- | ---- |
| [M5Unified](https://github.com/m5stack/M5Unified) `0.2.13` | M5Stack ハードウェア抽象化 |
| [M5GFX](https://github.com/m5stack/M5GFX) `0.2.19` | グラフィックス（LVGL 互換性のため 0.2.19 に固定） |
| [LVGL](https://lvgl.io/) `^8.3.9` | GUI フレームワーク |
| [ArduinoJson](https://arduinojson.org/) `^6.21.0` | 設定ファイルの JSON 解析 |
| [CapsuleSampler](https://github.com/wararyo/CapsuleSampler) | オーディオ合成エンジン |

ESP-IDF 側のコンポーネント設定は `sdkconfig.defaults` および `sdkconfig.m5stack-cores3` にまとまっています。

## プロジェクト構成

```
CapsuleChord2/
├── platformio.ini          # ビルド環境定義
├── sdkconfig.*             # ESP-IDF の設定
├── data/                   # LittleFS に書き込む音色データ
│   ├── piano/  aguitar/  ebass/  epiano/  supersaw/  popdrumkit/
├── include/                # ヘッダ用ディレクトリ
├── src/                    # アプリケーション本体
│   ├── main.cpp            # エントリポイント
│   ├── App/                # 各アプリ（演奏画面、ドラムパッド、シーケンサー…）
│   ├── Widget/             # カスタム LVGL ウィジェット
│   ├── KeyMap/             # キー配列 → 音楽イベント変換
│   ├── Output/             # 内蔵音源（CapsuleSampler ラッパ）
│   ├── Foundation/         # 音楽的基礎（時間、テンポ など）
│   ├── Assets/             # 自動演奏曲データ（MIDI から変換）
│   ├── Sample/  Fonts/     # 音色プリセット定義 / フォント
│   ├── Chord.{cpp,h}       # コード表現
│   ├── Scale.{cpp,h}       # スケール表現
│   ├── ChordPipeline.*     # キー入力 → 音声出力のルーティング
│   ├── Tempo.*             # テンポ管理 / ティック通知
│   ├── Keypad.*            # キーパッド入力（I2C）
│   ├── LvglWrapper.*       # LVGL 初期化
│   ├── BLEMidi.*           # BLE MIDI
│   ├── LittleFSManager.*   # 音色データの読み込み
│   └── SettingsStore.*     # NVS への設定保存
├── test/                   # PC 上ユニットテスト
└── tools/                  # MIDI → 楽曲データ変換スクリプト（Python）
```

主要なサブシステムの詳細は [`CLAUDE.md`](./CLAUDE.md) にもまとまっています。

## 外部キーパッドとの通信

演奏用キーパッドは別マイコンで動作する独立デバイスで、本体（CoreS3）とは I2C
（アドレス `0x09`）で接続されます。プロトコルは 2 系統あり、本体側は起動時に
キーパッドのファームウェア版を検出して自動で切り替えます。

- **v3.0 以降**: SMBus 風のレジスタアクセス方式
  - `0x00` ファームウェアバージョン（読み出し）
  - `0x70 + keyCode` 各キー LED の明るさ（読み書き）
  - `0xC8` 全 LED 一括制御（書き込み専用、`0x00` で全消灯）
  - `0xD0` キーイベント FIFO（読み出すと 1 件 pop。空なら `0x00`）
- **v2.x（レガシー）**: コマンドコード方式（`0x01` でキーイベント取得、`0x02` で LED 設定）

キーイベントは 1 バイトに圧縮されており、ビット構成は次のとおりです。

```
 7   6   5   4   3   2   1   0
[S] [  Group (3bit)  ] [Button (4bit)]
 │
 └─ 0: Released, 1: Pressed
```

Group は左ブロック / 右ブロック / その他（L, R, LT, RT 等）の 3 種類があり、
Button と組み合わせて最終的なキーコード（例: `0x11` = 右ブロックの 1 番）になります。
詳細な定義は [`src/Keypad.h`](./src/Keypad.h) を参照してください。

## MIDI から自動演奏曲データを作る

`tools/midi_to_capsule_chord.py` を使うと、MIDI ファイルを `AppAutoPlay` 用の C++ データに変換できます。
詳細は [`CLAUDE.md`](./CLAUDE.md) の「MIDI 変換ツール」セクションを参照してください。

## トラブルシューティング

- **書き込み時に COM ポートが見つからない**
  USB-C ケーブルがデータ通信対応のものか確認してください（充電専用ケーブルでは認識されません）。  
  Windows ではドライバが自動インストールされない場合があります。  
  また、CapsuleChord 2のUSB MIDI機能が有効になっている場合は書き込みが出来ません。  
  ホームボタン(端末下部中央のボタン)を長押ししながら起動し、アプリ一覧が表示されるまで長押しを続けることでUSB MIDI機能を無効にすることができます。

- **音が鳴らない / 起動時にエラー**
  LittleFS（`data/`）を書き込み忘れている可能性が高いです。
  上記「LittleFS（音色データ）の書き込み」の手順を実行してください。

- **Linux で `LIBUSB_ERROR_ACCESS` が出る**
  PlatformIO の udev ルール設定が必要です。詳細は [`CLAUDE.md`](./CLAUDE.md) を参照してください。
