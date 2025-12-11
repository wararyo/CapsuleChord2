#pragma once

#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <Sampler.h>
#include "LittleFSManager.h"

using namespace capsule::sampler;

/**
 * Timbre JSONファイル構造
 * {
 *   "name": "音色名",
 *   "samples": [
 *     {
 *       "lower-note-no": "ノートナンバー下限",
 *       "upper-note-no": "ノートナンバー上限",
 *       "lower-velocity": "ベロシティ下限",
 *       "upper-velocity": "ベロシティ上限",
 *       "sample": {
 *         "path": "WAVファイルへのパス",
 *         "root": "WAVファイルの音高",
 *         "loop-start": "ループ開始位置",
 *         "loop-end": "ループ終了位置",
 *         "adsr-enabled": "ADSR有効/無効",
 *         "attack": "アタック",
 *         "decay": "ディケイ",
 *         "sustain": "サスティン",
 *         "release": "リリース"
 *       }
 *     }
 *   ]
 * }
 */

typedef struct {
    // The "RIFF" chunk descriptor
    uint8_t ChunkID[4];
    int32_t ChunkSize;
    uint8_t Format[4];
    // The "fmt" sub-chunk
    uint8_t Subchunk1ID[4];
    int32_t Subchunk1Size;
    int16_t AudioFormat;
    int16_t NumChannels;
    int32_t SampleRate;
    int32_t ByteRate;
    int16_t BlockAlign;
    int16_t BitsPerSample;
} wav_header_t;

typedef struct {
    // The "data" sub-chunk
    uint8_t SubchunkID[4];
    int32_t SubchunkSize;
} wav_subchunk_header_t;

class WavFile {
public:
    WavFile() : file(nullptr), valid(false) {}
    WavFile(FILE* f, wav_header_t h, wav_subchunk_header_t sh)
        : file(f), header(h), subchunkHeader(sh), valid(true) {}
    ~WavFile() {}

    /**
     * @brief WAVファイルを開く
     * @param path ファイルパス（VFSマウントポイントを含む）
     * @return WavFileオブジェクト
     */
    static WavFile open(const char *path);

    /**
     * @brief WAVファイルを閉じる
     */
    void close();

    /**
     * @brief WAVファイルのヘッダー部分を除いたサイズを取得する
     * @return サイズ
     */
    size_t getDataSize();

    /**
     * @brief WAVファイルの長さをサンプル単位で取得する
     */
    size_t getSampleLength();

    /**
     * @brief WAVファイルからデータを読み込む
     * @param data 読み込んだデータを格納するバッファ
     * @param size バッファのサイズ
     * @return 読み込んだバイト数
     */
    size_t read(int16_t *data, size_t size);

    bool isValid() { return valid; }

private:
    FILE* file;
    bool valid;
    wav_header_t header;
    wav_subchunk_header_t subchunkHeader;
};

// LittleFSから音源データを読み込むクラス
class TimbreLoader {
public:
    TimbreLoader() {}
    ~TimbreLoader() {}

    /**
     * @brief 音源データを読み込む
     * @param path JSONファイルのファイルパス（LittleFSマウントポイントからの相対パス）
     * @note LittleFSは事前にmountLittleFS()でマウントしておく必要がある
     */
    std::shared_ptr<Timbre> loadTimbre(const char *path);
};

extern TimbreLoader Loader;
