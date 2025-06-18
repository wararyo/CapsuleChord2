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
