#pragma once
#include <stdint.h>

// 音楽的な時間
// 4分音符ひとつを480分割した長さを単位時間とする
typedef int32_t musical_time_t;

// 拍
typedef int32_t beats_t;

beats_t get_beats_from_musical_time(musical_time_t time);
musical_time_t get_musical_time_from_beats(beats_t beats);

musical_time_t time_in_bar(musical_time_t time); // TODO: 4/4拍子以外以外にも対応する
