#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <ArduinoJson.h>
#include "TimbreLoader.h"

static const char* LOG_TAG = "TimbreLoader";

WavFile WavFile::open(const char *path)
{
    // VFSフルパスを構築
    std::string fullPath = std::string(LITTLEFS_MOUNT_POINT) + path;

    FILE* file = fopen(fullPath.c_str(), "rb");
    if (file == nullptr) {
        ESP_LOGE(LOG_TAG, "Failed to open file: %s", fullPath.c_str());
        return WavFile();
    }

    wav_header_t header;
    size_t bytes_read = fread(&header, 1, sizeof(wav_header_t), file);
    if (bytes_read != sizeof(wav_header_t)) {
        ESP_LOGE(LOG_TAG, "Failed to read WAV header");
        fclose(file);
        return WavFile();
    }

    if (strncmp((const char *)header.Format, "WAVE", 4) != 0) {
        ESP_LOGE(LOG_TAG, "Invalid WAV format");
        fclose(file);
        return WavFile();
    }

    wav_subchunk_header_t subchunkHeader;
    while (true) {
        bytes_read = fread(&subchunkHeader, 1, sizeof(wav_subchunk_header_t), file);
        if (bytes_read != sizeof(wav_subchunk_header_t)) {
            ESP_LOGE(LOG_TAG, "Failed to read subchunk header");
            fclose(file);
            return WavFile();
        }
        if (strncmp((const char *)subchunkHeader.SubchunkID, "data", 4) == 0) {
            break;
        }
        // Skip unknown subchunks
        fseek(file, subchunkHeader.SubchunkSize, SEEK_CUR);
    }

    return WavFile(file, header, subchunkHeader);
}

void WavFile::close()
{
    if (file != nullptr) {
        fclose(file);
        file = nullptr;
    }
}

size_t WavFile::getDataSize()
{
    return subchunkHeader.SubchunkSize;
}

size_t WavFile::getSampleLength()
{
    return subchunkHeader.SubchunkSize / header.NumChannels / (header.BitsPerSample / 8);
}

size_t WavFile::read(int16_t *data, size_t size)
{
    if (file == nullptr) {
        return 0;
    }
    return fread(data, 1, size, file);
}

std::shared_ptr<Timbre> TimbreLoader::loadTimbre(const char *path)
{
    // LittleFSがマウントされているか確認
    if (!isLittleFSMounted()) {
        ESP_LOGE(LOG_TAG, "LittleFS not mounted. Call mountLittleFS() first.");
        return nullptr;
    }

    // VFSフルパスを構築
    std::string fullPath = std::string(LITTLEFS_MOUNT_POINT) + path;

    FILE* file = fopen(fullPath.c_str(), "r");
    if (file == nullptr) {
        ESP_LOGE(LOG_TAG, "Failed to open JSON file: %s", fullPath.c_str());
        return nullptr;
    }

    // ファイルサイズを取得
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // ファイル内容を読み込む
    std::vector<char> jsonBuffer(fileSize + 1);
    size_t bytesRead = fread(jsonBuffer.data(), 1, fileSize, file);
    jsonBuffer[bytesRead] = '\0';
    fclose(file);

    // JSONをパース
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, jsonBuffer.data());
    if (error) {
        ESP_LOGE(LOG_TAG, "Failed to parse JSON: %s", error.c_str());
        return nullptr;
    }

    // ディレクトリパスを抽出
    std::string pathStr(path);
    std::string directoryPath = pathStr.substr(0, pathStr.find_last_of('/'));

    // サンプルを読み込む
    auto samples = std::make_unique<std::vector<std::unique_ptr<Timbre::MappedSample>>>();

    JsonArray samplesJson = doc["samples"].as<JsonArray>();
    for (JsonVariant sampleJson : samplesJson) {
        const uint8_t lowerNoteNo = sampleJson["lower-note-no"];
        const uint8_t upperNoteNo = sampleJson["upper-note-no"];
        const uint8_t lowerVelocity = sampleJson["lower-velocity"];
        const uint8_t upperVelocity = sampleJson["upper-velocity"];
        const char *samplePath = sampleJson["sample"]["path"];
        const uint8_t root = sampleJson["sample"]["root"];
        const uint32_t loopStart = sampleJson["sample"]["loop-start"];
        const uint32_t loopEnd = sampleJson["sample"]["loop-end"];
        const bool adsrEnabled = sampleJson["sample"]["adsr-enabled"];
        const float attack = sampleJson["sample"]["attack"];
        const float decay = sampleJson["sample"]["decay"];
        const float sustain = sampleJson["sample"]["sustain"];
        const float release = sampleJson["sample"]["release"];

        std::string fullSamplePath = directoryPath + "/" + samplePath;
        WavFile wavFile = WavFile::open(fullSamplePath.c_str());
        if (!wavFile.isValid()) {
            ESP_LOGE(LOG_TAG, "Failed to open wav file: %s", fullSamplePath.c_str());
            return nullptr;
        }

        size_t dataSize = wavFile.getDataSize();
        int16_t *data = (int16_t *)heap_caps_malloc(dataSize, MALLOC_CAP_SPIRAM);
        if (!data) {
            ESP_LOGE(LOG_TAG, "Failed to allocate %zu bytes for sample: %s", dataSize, fullSamplePath.c_str());
            wavFile.close();
            return nullptr;
        }

        size_t written_bytes = wavFile.read(data, dataSize);
        assert(written_bytes == dataSize);
        size_t sampleLength = wavFile.getSampleLength();

        // unique_ptrでメモリを管理
        std::unique_ptr<const int16_t> sampleData(data);
        std::shared_ptr<Sample> s = std::make_shared<Sample>(
            std::move(sampleData), sampleLength, root,
            loopStart, loopEnd,
            adsrEnabled, attack, decay, sustain, release);

        auto ms = std::make_unique<Timbre::MappedSample>(s, lowerNoteNo, upperNoteNo, lowerVelocity, upperVelocity);
        samples->push_back(std::move(ms));
        wavFile.close();
    }

    if (samples->empty()) {
        ESP_LOGW(LOG_TAG, "No samples found in: %s", path);
        return nullptr;
    }

    ESP_LOGI(LOG_TAG, "Loaded timbre: %s (%zu samples)", path, samples->size());
    return std::make_shared<Timbre>(samples.release());
}

TimbreLoader Loader;
