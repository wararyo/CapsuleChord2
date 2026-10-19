# 起動時間短縮プラン

電源ボタンを押してから演奏が可能になるまでの時間を短くするための調査結果とアイデア集。
2026-09-10 時点の `feature/custom-key-settings` ブランチのコードと設定を読んで作成した。
所要時間の数値はすべて実測前の概算であり、まず計測を入れて確かめること。

## 1. 現状の起動フローとコスト

### 1.1 起動の流れ

```text
電源ボタン押下
  → AXP2101 が ONLEVEL (128ms) の長押しを検出して電源ON
  → ESP32-S3 ROM ブートローダ
  → 2nd stage ブートローダ (ログ出力、アプリイメージ SHA-256 検証)
  → app 起動 (PSRAM 初期化 + メモリテスト、ヒープ初期化)
  → app_main → setup()
      M5.begin()              : PMIC / AW9523 / 表示パネル / タッチ / IMU / RTC 初期化
      Lvgl.begin()
      Keypad.begin()          : イベントキューのドレイン、FW バージョン検出
      I2C.begin()             : I2C タスク開始
      mountLittleFS()         : マウント + 使用量集計ログ
      Settings.loadAll()
      Display.begin()
      Output.initializeOutput(): 音色スキャン → bass/drum/main を同期ロード → I2S 初期化 (+100ms 待ち)
      Tempo.begin()
      playScreen.create()
      UsbComposite::begin()
  → loop() 開始、最初の描画
```

### 1.2 確定している「無駄」

コードと設定から、確実に起動を遅くしていると言えるもの。

| 項目 | 場所 | 推定コスト |
|---|---|---|
| PSRAM 8MB のメモリテスト | `CONFIG_SPIRAM_MEMTEST=y` | 公式ドキュメントで「4MB あたり約 1 秒」。CoreS3 は 8MB |
| BMI270 の設定ファイル約 8KB を I2C で転送 | `M5.begin()` の `internal_imu=true` 既定値 | 200〜300ms |
| ログ出力 UART 115200bps、ブートローダ/アプリとも INFO | `CONFIG_BOOTLOADER_LOG_LEVEL_INFO`, `CONFIG_LOG_DEFAULT_LEVEL=3` | 数百 ms。1 行 100 文字で約 9ms、送信はブロッキング |
| I2S 初期化直後の固定待ち | `src/Output/OutputInternal.cpp` `initAudioOutput()` 内の `esp_delay(100)` | 100ms |
| アプリイメージ 1.65MB の SHA-256 検証 | ブートローダ既定 | 100〜300ms |
| 音色 WAV 約 840KB を全部同期ロード | `OutputInternal::begin()` で bass/drum/main を順に読む | 150〜400ms |
| LittleFS の使用量集計ログ | `mountLittleFS()` の `esp_littlefs_info()` は全ブロック走査 | 数十 ms |
| ILI9342C の Sleep Out 後 120ms 待ち | M5GFX `Panel_ILI9342` の初期化コマンド列にハードコード | 約 100ms 削減可能 |
| デバッグビルド | `build_type = debug`、`-Og` | 全体が 1〜3 割遅い |

## 2. アイデア一覧

### A. sdkconfig だけで済むもの (効果大、リスク小)

1. `CONFIG_SPIRAM_MEMTEST=n`
   最大の一手。おそらく 1 秒以上縮む。
2. `CONFIG_BOOTLOADER_SKIP_VALIDATE_ON_POWER_ON=y`
   電源投入時だけイメージ検証を飛ばす。`SKIP_VALIDATE_ALWAYS` はフラッシュ破損時に起動不能になるので避ける。
3. ログレベルの引き下げ、またはボーレート引き上げ
   - ブートローダログを `NONE` か `ERROR` に、アプリの `CONFIG_LOG_DEFAULT_LEVEL` を `WARN` に。
     デバッグ時は `esp_log_level_set()` で起動後に戻せる (`CONFIG_LOG_MAXIMUM_LEVEL` は上げておく)。
   - もしくは `CONFIG_ESP_CONSOLE_UART_BAUDRATE=921600`。出力量を変えずに 8 倍速くなる。
     ブートローダにも適用される。DebugProbe 側のモニタ速度も変える必要があり、`CLAUDE.md` の手順も更新する。
4. `CONFIG_SPIRAM_SPEED_80M`
   M5Stack 公式ドキュメントで CoreS3 の PSRAM は Quad 80MHz と明記されている。
   音色コピーと音声処理の両方が速くなる。
5. `CONFIG_ESPTOOLPY_FLASHMODE_QIO`
   PlatformIO の CoreS3 ボード定義は Arduino 側で `qio` を使っているので、フラッシュチップは対応している。
   XIP のキャッシュミスと LittleFS 読み出しが速くなる。
6. 命令キャッシュ 32KB (`CONFIG_ESP32S3_INSTRUCTION_CACHE_32KB`)、データキャッシュ 64KB (`CONFIG_ESP32S3_DATA_CACHE_64KB`)
   内部 RAM を食うので空き容量を確認してから。
7. `CONFIG_BOOTLOADER_COMPILER_OPTIMIZATION_PERF`
   ブートローダ自体は数十 ms なので効果は小。
8. `CONFIG_RTC_CLK_CAL_CYCLES=0`
   テンポは esp_timer 基準なので影響なし。効果は数 ms。
9. 未使用コンポーネントの無効化
   `CONFIG_ESP_WIFI_ENABLED`、lwIP、mbedTLS 等が `y` のままリンクされている。
   無効化するとイメージが縮み、検証を残す場合の時間も減る。NimBLE が mbedTLS に依存している点は要確認。

### B. `M5.begin()` の設定変更

10. `cfg.internal_imu = false`
    IMU は `AppBall` だけが使う。Ball 起動時に I2C タスク上で `M5.Imu.begin(&M5.In_I2C, M5.getBoard())` を遅延実行する。
11. `cfg.internal_rtc = false`
    RTC からの時刻同期は演奏に不要。
12. `cfg.internal_mic = false`、`cfg.internal_spk = false`
    I2S は `OutputInternal` が自前で組んでいるので不要。
13. `cfg.external_display_value = 0`
    既定では Port A の I2C 上で OLED/LCD ユニットを探しに行く。
    これはキーパッドと同じバスなので、速度以外の理由でも触らない方が安全。
14. `cfg.clear_display = false`
    LVGL 側で黒背景を塗るので、M5GFX の全面クリアは省略できる。数十 ms。
15. ILI9342C の Sleep Out 待ち短縮
    データシート上、Sleep Out 後の次コマンドまでは 5ms で良い。
    `Panel_M5StackCoreS3` を継承して初期化コマンド列を差し替えれば約 100ms 削れる。
    表示が一瞬乱れる可能性があるので優先度は低め。

### C. `setup()` の並列化・順序・遅延ロード

16. 音色ロードを Core0 で並列実行
    `mountLittleFS()` を `M5.begin()` より前に移し、直後に Core0 で音色ロードタスクを起こす。
    フラッシュ読みは I2C を使わないので、Core1 で走る M5.begin/表示初期化と完全に重なる。
    `Output.Internal.begin()` はイベントグループで完了を待つだけにする。
17. main 音色を最優先、bass/drum はバックグラウンド
    演奏に必要なのは main だけなので「演奏可能」までの時間が直接縮む。
    シーケンサー/ドラム系アプリの起動時にロード完了を待つ仕組みが必要。
18. `esp_delay(100)` の削除または移動
    待つ理由が AW88298 の安定待ちなら別タスクや `update()` 側に移す。
19. `esp_littlefs_info()` をデバッグビルド限定にする。
20. 音色を 1 つのバンク形式にまとめる
    WAV 個別ファイルより open/parse の回数が減る。効果は小さめ。
21. サンプルの `esp_partition_mmap` によるゼロコピー参照
    専用パーティションに置いてフラッシュから直接参照すれば、ロード時間はほぼゼロになる。
    ただしフラッシュキャッシュ経由のランダムアクセスは PSRAM より遅く、
    32 音同時発音で音切れの恐れがある。要実験。
22. `Settings.loadAll()` の分割
    Output 以外のカテゴリは PlayScreen 表示直前まで遅らせても良い。
23. リリース環境 `m5stack-cores3-release` を常用する。

### D. ハードウェア・電源・キーパッド側

24. AXP2101 の電源キー長押し判定
    レジスタ 0x27 の ONLEVEL (bit1:0) で 128ms / 512ms / 1s / 2s から選べる。
    M5Unified は 0x27 に 0x00 を書くので既に最短の 128ms になっている。これ以上は縮まらない。
    この値はファームが書き込んで PMIC 内に保持されるものなので、電池を外した個体は初回だけ長くなる。
25. 電源 OFF の代わりに light sleep
    電源ボタンで復帰させれば PSRAM も RAM も保持されるので復帰はほぼ瞬時。
    数 mA の待機電流が増えるので「無操作 N 時間は sleep、その後 powerOff」の 2 段構えが現実的。
    deep sleep は PSRAM が消えるので音色再ロードが必要になり、メリットが薄い。
26. キーパッド側 (RP2040 + Picoboot3)
    Picoboot3 は BOOTSEL3 ピンを読むまで 5ms 待つだけで、そのままアプリにジャンプする。
    RP2040 側に大きな遅延はない。
27. ESP32 側を速くした場合のキーパッド検出タイミング
    `Keypad.begin()` のバージョン問い合わせがキーパッド起動前に走ると Legacy にフォールバックする。
    キーパッドの 5V が `Power.setExtOutput()` で入る構成なら、M5.begin から Keypad.begin までは数十 ms しかない。
    バージョン検出を数回リトライするか、失敗時に I2C タスク側で再検出する仕組みを入れておく。

### E. 体感を速くする工夫

28. 初回描画を最小限にして、コード名だけ先に出す。残りのウィジェットは 1 ループ後に作る。
29. 演奏可能になった瞬間にキーパッド LED を点灯させる、または短い起動音を鳴らす。
    ユーザーが「押していい」と分かるタイミングが明確になる。

## 3. 進め方

### 3.1 計測を先に入れる

`setup()` の各ステップ後に `esp_timer_get_time()` を出す 1 行ログを挟むだけで、アプリ側の内訳は分かる。
`docs/architecture-improvement-plan.md` で提案されている起動メモリログと同じ場所に置ける。
ブートローダ以前は電源ボタン押下からのスマホ動画 (60fps) で十分。

ESP_LOG のタイムスタンプは app 起動時点が 0 なので、ブートローダとメモリテストの時間は含まれない点に注意。

### 3.2 推奨する順序

| フェーズ | 内容 | 見込み |
|---|---|---|
| 1 | A-1〜3、B-10〜13、C-18〜19 | コード変更が小さいまま 1.5〜2 秒程度 |
| 2 | A-4〜6、C-23 | 全体の底上げ |
| 3 | C-16〜17 | 並列化と遅延ロード。演奏可能までの時間を直接縮める |
| 4 | D-27 | 高速化に伴うキーパッド検出の堅牢化 |
| 5 | B-15、C-21、D-25 | 効果はあるがリスクや設計変更を伴うもの。要実験 |

## 4. 参考資料

- [ESP-IDF Speed Optimization / Startup Time](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-guides/performance/speed.html)
- [ESP-IDF Kconfig Reference](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/kconfig-reference.html)
- [AXP2101 Datasheet (M5Stack)](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/core/Core2%20v1.1/axp2101.pdf)
- [M5Stack CoreS3 Docs](https://docs.m5stack.com/en/core/CoreS3)
- [Picoboot3 README](https://github.com/IndoorCorgi/picoboot3)
- M5Unified `M5Unified.cpp` `_begin()` / `_begin_rtc_imu()`、`utility/imu/BMI270_Class.cpp`
- M5GFX `M5GFX.cpp` autodetect、`lgfx/v1/panel/Panel_ILI9342.hpp`
