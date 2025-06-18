#!/usr/bin/env python3
"""
MIDIファイルからCapsuleChord2用の曲データを生成するツール

使用方法:
python midi_to_capsule_chord.py --input song.mid --song_name MySong --output_dir ../src/Assets
"""

import mido
import argparse
import os
from collections import defaultdict

def detect_key_from_midi(midi):
    """MIDIファイルからキーを検出する"""
    # キー名とその相対的な音程
    key_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    
    # チャンネル1のノートを収集
    channel_1_notes = []
    
    for track in midi.tracks:
        for msg in track:
            if hasattr(msg, 'channel') and msg.channel == 0:  # チャンネル1
                if msg.type == 'note_on' and msg.velocity > 0:
                    channel_1_notes.append(msg.note % 12)  # オクターブを無視
    
    if not channel_1_notes:
        print("警告: チャンネル1にノートが見つかりません。Cメジャーを仮定します。")
        return 0, 'C'
    
    # 各キーに対してスコアを計算
    key_scores = {}
    
    for root_note in range(12):
        key_name = key_names[root_note]
        # メジャースケールの音程パターン (W-W-H-W-W-W-H)
        major_scale = [(root_note + offset) % 12 for offset in [0, 2, 4, 5, 7, 9, 11]]
        
        # このキーでのスコア計算
        score = 0
        for note in channel_1_notes:
            if note in major_scale:
                # トニック、ドミナント、サブドミナントに重み付け
                if note == major_scale[0]:  # I (トニック)
                    score += 3
                elif note == major_scale[4]:  # V (ドミナント)
                    score += 2
                elif note == major_scale[3]:  # IV (サブドミナント)
                    score += 2
                else:
                    score += 1
        
        key_scores[root_note] = score
    
    # 最高スコアのキーを選択
    detected_key = max(key_scores.keys(), key=lambda k: key_scores[k])
    detected_key_name = key_names[detected_key]
    
    print(f"キー検出結果: {detected_key_name}メジャー (信頼度: {key_scores[detected_key]}/{len(channel_1_notes)})")
    
    return detected_key, detected_key_name

def note_to_degree_chord(note, key_root):
    """MIDIノート番号を指定されたキーのディグリーコードに変換"""
    note_in_key = (note - key_root) % 12
    
    # メジャースケールでの各音程に対応するディグリー
    degree_map = {
        0: 'DegreeChord::I',     # ルート
        2: 'DegreeChord::II',    # 2度
        4: 'DegreeChord::III',   # 3度
        5: 'DegreeChord::IV',    # 4度
        7: 'DegreeChord::V',     # 5度
        9: 'DegreeChord::VI',    # 6度
        11: 'DegreeChord::VII'   # 7度
    }
    
    return degree_map.get(note_in_key, 'DegreeChord::I')

def analyze_chord_type(notes, key_root):
    """複数の音符からコードタイプを分析"""
    if not notes:
        return 'DegreeChord::I', 'Major'
    
    # 音符を正規化（オクターブを無視し、キーに対する相対音程に変換）
    normalized_notes = sorted(set((note - key_root) % 12 for note in notes))
    
    # ルート音を決定（最低音を基準とする）
    root_note = min(normalized_notes)
    
    # ルート音からの相対音程を計算
    intervals = sorted(set((note - root_note) % 12 for note in normalized_notes))
    
    # コードタイプを判定
    if intervals == [0, 4, 7]:  # Major triad
        chord_type = 'Major'
    elif intervals == [0, 3, 7]:  # Minor triad  
        chord_type = 'Minor'
    elif intervals == [0, 3, 6]:  # Diminished triad
        chord_type = 'Dimish'  # Chord.hの定義に合わせる
    elif intervals == [0, 4, 8]:  # Augmented triad
        chord_type = 'Aug'
    elif intervals == [0, 5, 7]:  # Sus4
        chord_type = 'Sus4'
    elif intervals == [0, 2, 7]:  # Sus2
        chord_type = 'Sus2'
    elif set(intervals).issuperset({0, 4, 7, 10}):  # Dominant 7th
        chord_type = 'Major | Chord::Seventh'
    elif set(intervals).issuperset({0, 4, 7, 11}):  # Major 7th
        chord_type = 'Major | Chord::MajorSeventh'
    elif set(intervals).issuperset({0, 3, 7, 10}):  # Minor 7th
        chord_type = 'Minor | Chord::Seventh'
    elif set(intervals).issuperset({0, 4, 7, 9}):  # Major 6th
        chord_type = 'Major | Chord::Sixth'
    elif set(intervals).issuperset({0, 3, 7, 9}):  # Minor 6th
        chord_type = 'Minor | Chord::Sixth'
    else:
        # デフォルトはMajor（複雑なコードや不完全なコード）
        chord_type = 'Major'
    
    # ディグリーコードを決定
    degree_chord = note_to_degree_chord(root_note + key_root, key_root)
    
    return degree_chord, chord_type

def generate_header_file(song_name, key_name, output_dir):
    """ヘッダーファイルを生成"""
    header_content = f'''#pragma once

#include "../App/AppAutoPlay.h"
#include "DemoSong.h" // 基本定義を使用

// {song_name} ({key_name}メジャーキー)
extern const AppAutoPlay::AutoPlayCommand {song_name.upper()}_COMMANDS[];
extern const size_t {song_name.upper()}_COMMAND_COUNT;
extern const musical_time_t {song_name.upper()}_DURATION;
extern const TempoController::tempo_t {song_name.upper()}_TEMPO;
extern const uint8_t {song_name.upper()}_KEY;
'''
    
    header_file_path = os.path.join(output_dir, f"{song_name}.h")
    with open(header_file_path, 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    return header_file_path

def generate_cpp_file(song_name, commands, tempo, duration, key_root, key_name, output_dir):
    """CPPファイルを生成"""
    cpp_content = f'''#include "{song_name}.h"

// {song_name} ({key_name}メジャーキー)
const AppAutoPlay::AutoPlayCommand {song_name.upper()}_COMMANDS[] = {{
'''
    
    # コマンドを時間順にソート
    commands.sort(key=lambda x: x[0])
    
    for time, command_type, data in commands:
        if command_type == 'CHORD_START':
            degree_chord, chord_type = data
            cpp_content += f'    {{{time}, DegreeChord({degree_chord}, Chord::{chord_type}), AppAutoPlay::CommandType::CHORD_START}},\n'
        elif command_type == 'CHORD_END':
            cpp_content += f'    {{{time}, DegreeChord(), AppAutoPlay::CommandType::CHORD_END}},\n'
        elif command_type == 'MIDI_NOTE':
            status, data1, data2 = data
            cpp_content += f'    {{{time}, 0x{status:02X}, {data1}, {data2}}},\n'
    
    cpp_content += f'''}};

const size_t {song_name.upper()}_COMMAND_COUNT = sizeof({song_name.upper()}_COMMANDS) / sizeof({song_name.upper()}_COMMANDS[0]);
const musical_time_t {song_name.upper()}_DURATION = {duration};
const TempoController::tempo_t {song_name.upper()}_TEMPO = {tempo};
const uint8_t {song_name.upper()}_KEY = {key_root}; // {key_name}
'''
    
    cpp_file_path = os.path.join(output_dir, f"{song_name}.cpp")
    with open(cpp_file_path, 'w', encoding='utf-8') as f:
        f.write(cpp_content)
    
    return cpp_file_path

def main():
    parser = argparse.ArgumentParser(description="MIDIファイルからCapsuleChord2用の曲データを生成")
    parser.add_argument("--input", type=str, required=True, help="入力MIDIファイル")
    parser.add_argument("--song_name", type=str, required=True, help="楽曲名（ファイル名に使用）")
    parser.add_argument("--output_dir", type=str, default="../src/Assets", help="出力ディレクトリ")
    parser.add_argument("--ppq", type=int, default=480, help="musical_time_tの1/4音符あたりの値（デフォルト: 480）")
    
    args = parser.parse_args()
    
    # MIDIファイルを読み込み
    try:
        midi = mido.MidiFile(args.input)
    except Exception as e:
        print(f"MIDIファイルの読み込みエラー: {e}")
        return
    
    print(f"MIDIファイル読み込み: {args.input}")
    print(f"Ticks per beat: {midi.ticks_per_beat}")
    print(f"楽曲名: {args.song_name}")
    
    # キーを検出
    key_root, key_name = detect_key_from_midi(midi)
    
    # 解析用変数
    commands = []
    tempo = 120  # デフォルトテンポ
    accumulated_time = 0
    track_times = defaultdict(int)  # トラック別の累積時間
    
    # コード追跡用の構造体
    active_chord_notes = set()  # 現在アクティブなコードの音符
    last_chord_time = 0  # 最後にコードが変更された時間
    last_chord_data = None  # 最後のコード情報 (degree_chord, chord_type)
    
    # 各トラックを処理
    for track_num, track in enumerate(midi.tracks):
        current_time = 0
        
        for msg in track:
            current_time += msg.time
            musical_time = (current_time * args.ppq) // midi.ticks_per_beat
            
            if msg.is_meta:
                if msg.type == 'set_tempo':
                    # テンポ変更
                    tempo = round(60000000 / msg.tempo)  # マイクロ秒/拍からBPMに変換
                    print(f"テンポ設定: {tempo} BPM")
                elif msg.type == 'end_of_track':
                    accumulated_time = max(accumulated_time, musical_time)
            
            elif hasattr(msg, 'channel'):
                if msg.channel == 0:  # チャンネル1（コード）
                    chord_changed = False
                    
                    if msg.type == 'note_on' and msg.velocity > 0:
                        # 音符を追加
                        active_chord_notes.add(msg.note)
                        chord_changed = True
                        
                    elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                        # 音符を削除
                        active_chord_notes.discard(msg.note)
                        chord_changed = True
                    
                    # コードが変更された場合の処理
                    if chord_changed:
                        # 前のコードを終了
                        if last_chord_data is not None:
                            commands.append((musical_time, 'CHORD_END', None))
                        
                        # 新しいコードを開始（音符が存在する場合）
                        if active_chord_notes:
                            degree_chord, chord_type = analyze_chord_type(list(active_chord_notes), key_root)
                            commands.append((musical_time, 'CHORD_START', (degree_chord, chord_type)))
                            last_chord_data = (degree_chord, chord_type)
                            last_chord_time = musical_time
                            print(f"時刻 {musical_time}: {degree_chord} {chord_type} (音符: {sorted(active_chord_notes)})")
                        else:
                            last_chord_data = None
                
                elif msg.channel == 9:  # チャンネル10（ドラム）
                    if msg.type == 'note_on' and msg.velocity > 0:
                        # ドラム音符
                        status = 0x90 | msg.channel  # Note On + チャンネル
                        commands.append((musical_time, 'MIDI_NOTE', (status, msg.note, msg.velocity)))
                    elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                        # ドラムのNote Off（通常は無視するが、一応生成）
                        status = 0x80 | msg.channel  # Note Off + チャンネル  
                        commands.append((musical_time, 'MIDI_NOTE', (status, msg.note, 0)))
    
    # 楽曲終了時に残っているコードを終了
    if last_chord_data is not None:
        commands.append((accumulated_time, 'CHORD_END', None))
    
    # 楽曲の長さを設定
    duration = accumulated_time
    
    print(f"生成されたコマンド数: {len(commands)}")
    print(f"楽曲の長さ: {duration} musical_time_t")
    print(f"最終テンポ: {tempo} BPM")
    
    # 出力ディレクトリを作成
    os.makedirs(args.output_dir, exist_ok=True)
    
    # ファイル生成
    header_file = generate_header_file(args.song_name, key_name, args.output_dir)
    cpp_file = generate_cpp_file(args.song_name, commands, tempo, duration, key_root, key_name, args.output_dir)
    
    print(f"\n生成されたファイル:")
    print(f"  ヘッダー: {header_file}")
    print(f"  実装: {cpp_file}")
    print(f"\n次のステップ:")
    print(f"1. AppAutoPlay.cppに #include \"../Assets/{args.song_name}.h\" を追加")
    print(f"2. initializeSongs()メソッドに楽曲データを追加")
    print(f"3. initializeSongs()で楽曲にキー情報({key_name}メジャー)を設定")

if __name__ == "__main__":
    main()
