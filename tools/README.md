# CapsuleChord2 MIDI変換ツール

MIDIファイルからCapsuleChord2用の曲データを生成するツールです。

## セットアップ

```bash
cd tools
python3 -m venv venv
source venv/bin/activate  # macOS/Linux
# または
venv\Scripts\activate     # Windows

pip install -r requirements.txt
```

## 使用方法

```bash
# 基本的な使用方法
python midi_to_capsule_chord.py --input song.mid --song_name MySong

# 出力ディレクトリを指定
python midi_to_capsule_chord.py --input song.mid --song_name MySong --output_dir ../src/Assets

# musical_time_tの分解能を指定
python midi_to_capsule_chord.py --input song.mid --song_name MySong --ppq 480
```

## パラメータ

- `--input`: 入力MIDIファイル（必須）
- `--song_name`: 楽曲名。ファイル名とC++の変数名に使用されます（必須）
- `--output_dir`: 出力ディレクトリ（デフォルト: ../src/Assets）
- `--ppq`: musical_time_tの1/4音符あたりの値（デフォルト: 480）

## MIDIファイルの要件

### チャンネル1（コード）
- コードのルート音が記録されている必要があります
- Note Onでコード開始、Note Offでコード終了として扱われます
- 現在はすべてMajorコードとして解釈されます

### チャンネル10（ドラム）
- General MIDIドラムマップに従ったドラムパートが記録されている必要があります
- すべてのノートイベントがMIDI_NOTEコマンドとして変換されます

## 出力ファイル

ツールは以下の2つのファイルを生成します：

1. `{song_name}.h` - ヘッダーファイル
2. `{song_name}.cpp` - 実装ファイル

## 生成後の統合手順

1. AppAutoPlay.cppに新しいヘッダーファイルをインクルード：
   ```cpp
   #include "../Assets/{song_name}.h"
   ```

2. `initializeSongs()`メソッドに新しい楽曲を追加：
   ```cpp
   availableSongs.push_back({
       "楽曲名",
       {SONG_NAME}_COMMANDS,
       {SONG_NAME}_COMMAND_COUNT,
       {SONG_NAME}_TEMPO,
       {SONG_NAME}_DURATION
   });
   ```

## 例

```bash
# sample.midファイルをTestSongとして変換
python midi_to_capsule_chord.py --input sample.mid --song_name TestSong
```

これにより以下のファイルが生成されます：
- `../src/Assets/TestSong.h`
- `../src/Assets/TestSong.cpp`

---

# ELF退避ツール (save_elf.py)

ビルド済みの `firmware.elf` を addr2line でのクラッシュ解析用に `firmware_backup/` へ退避します。
ファイル名は elf の SHA256 上位16桁になり、これはデバイス起動時/クラッシュ時にログへ出る
`ELF file SHA256:` の値と一致します。後からクラッシュログのハッシュで対応するelfを特定できます。

`firmware_backup/*.elf` は `.gitignore` 済みで、コミットされずローカルに保管されます。

## 使用方法

```bash
# デフォルト環境 (m5stack-cores3-release) の elf を退避
python tools/save_elf.py

# 環境名を指定して退避 (.pio/build/<env>/firmware.elf)
python tools/save_elf.py -e m5stack-cores3

# elf パスを直接指定 (-e より優先)
python tools/save_elf.py --elf path/to/firmware.elf
```

アップロード直後に実行してください。elf にはビルド日時が埋め込まれるため、
ソースが同じでもビルドのたびにハッシュは変わります。

## クラッシュ解析 (addr2line)

クラッシュログの `ELF file SHA256: xxxxxxxxxxxxxxxx ...` の16桁で対応elfを特定し、
バックトレースのアドレスをソース箇所に変換します。

```bash
xtensa-esp32s3-elf-addr2line -e firmware_backup/xxxxxxxxxxxxxxxx.elf -fpiaC 0x420xxxxx 0x420yyyyy
```

## 退避したelfから書き込む

elf → bin の変換は決定的なので、退避したelfから当時のファームを再現して焼き直せます。

```bash
# 1. elf から bin を生成
python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 \
  elf2image -o firmware.bin firmware_backup/xxxxxxxxxxxxxxxx.elf

# 2. app パーティション (デフォルト 0x10000) へ書き込み
python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 \
  -p <PORT> write_flash 0x10000 firmware.bin
```

書き込み時は **ホームボタンを押しながら電源ON** でJTAGモードにしてから実行してください。
bootloader / partition table は端末に焼かれたままなので、appパーティションへの書き込みだけで戻せます。
（appオフセットはパーティション構成依存です。デフォルトESP-IDFは `0x10000`）
