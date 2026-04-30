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

# SF2 → ティンバー変換ツール (`sf2_to_timbre.py`)

SoundFont 2 (.sf2) ファイルから CapsuleSampler 用のティンバー
(`data/timbres/<name>/` 配下の WAV と JSON) を生成します。

忠実な変換ではなく、おおまかにプレイアブルなティンバーへ近似変換するのが目的です。
- フィルタ / LFO / モジュレーションエンベロープは無視されます
- delay / hold エンベロープは無視されます
- ステレオサンプルはモノラルにダウンミックスされます
- pitch_correction / fineTune の端数は四捨五入で root note に丸められます
- 部分的に重なるゾーンはノート/ベロシティ軸で分割し、CapsuleSampler の
  「完全一致 or 完全分離」制約を満たすよう正規化されます

## 使用方法

```bash
# プリセット一覧を表示
python sf2_to_timbre.py --sf2 GeneralUser.sf2 --list

# bank=0 program=0 のプリセットを 'piano2' という名前で出力
python sf2_to_timbre.py --sf2 GeneralUser.sf2 --preset 0:0 --name piano2 --category keys
```

## パラメータ

- `--sf2`: 入力 SF2 ファイル（必須）
- `--list`: プリセット一覧を表示して終了
- `--preset`: プリセット指定。`bank:program` 形式 / プリセット名 / インデックスが使用可能
- `--name`: 出力ティンバー名（ディレクトリ名と JSON ファイル名に使われる）
- `--category`: ティンバーの category フィールド（default: `keys`）
- `--output-dir`: 出力先のティンバールート（default: `../data/timbres`）
