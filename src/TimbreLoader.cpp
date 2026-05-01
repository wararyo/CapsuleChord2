#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>
#include <cassert>
#include <dirent.h>
#include <sys/stat.h>
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
    DynamicJsonDocument doc(65536);
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

    // 同一WAVファイルが複数のSampleエントリ(ベロシティレイヤなど)から参照されることがあるため、
    // パスをキーにしたWAVバッファのキャッシュを持つ。各Sampleはshared_ptrで同じバッファを共有する。
    struct CachedWav {
        std::shared_ptr<const int16_t> data;
        size_t sampleLength; // WAVヘッダ由来の生のサンプル長(adsrEnabledによる短縮は適用前)
    };
    std::unordered_map<std::string, CachedWav> wavCache;

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
        const bool  adsrEnabled       = sampleJson["sample"]["adsr-enabled"]           | true;
        const float attack            = sampleJson["sample"]["attack"];
        const float decay             = sampleJson["sample"]["decay"];
        const float sustain           = sampleJson["sample"]["sustain"];
        const float release           = sampleJson["sample"]["release"];
        const bool  filterEnabled     = sampleJson["sample"]["filter-enabled"]         | false;
        const float filterCutoffCent  = sampleJson["sample"]["filter-cutoff-cent"]     | 13500.0f;
        const float filterResonance   = sampleJson["sample"]["filter-resonance"]       | 0.707f;
        const float filterEnvAmount   = sampleJson["sample"]["filter-env-amount-cent"] | 0.0f;
        const float filterAttack      = sampleJson["sample"]["filter-attack"]          | 1.0f;
        const float filterDecay       = sampleJson["sample"]["filter-decay"]           | 1.0f;
        const float filterSustain     = sampleJson["sample"]["filter-sustain"]         | 1.0f;
        const float filterRelease     = sampleJson["sample"]["filter-release"]         | 1.0f;

        std::string fullSamplePath = directoryPath + "/" + samplePath;

        // キャッシュヒットなら同じバッファを共有し、なければWAVを新規ロードしてキャッシュに登録
        std::shared_ptr<const int16_t> sampleData;
        size_t rawSampleLength;
        auto cacheIt = wavCache.find(fullSamplePath);
        if (cacheIt != wavCache.end()) {
            sampleData = cacheIt->second.data;
            rawSampleLength = cacheIt->second.sampleLength;
        } else {
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
            rawSampleLength = wavFile.getSampleLength();
            wavFile.close();

            // heap_caps_mallocで確保したメモリを正しく解放するため、カスタムデリータ付きshared_ptrでラップする
            sampleData = std::shared_ptr<const int16_t>(
                static_cast<const int16_t *>(data),
                [](const int16_t *p) { heap_caps_free(const_cast<int16_t *>(p)); });
            wavCache.emplace(fullSamplePath, CachedWav{sampleData, rawSampleLength});
        }

        size_t sampleLength = rawSampleLength;
        if (!adsrEnabled) {
            // 処理の高速化の都合上、ワンショット音源は後ろに1024サンプル程度の余白を設ける必要がある
            if (sampleLength > 1024) sampleLength -= 1024;
            else sampleLength = SAMPLE_BUFFER_SIZE;
        }

        std::shared_ptr<Sample> s = std::make_shared<Sample>(
            sampleData, sampleLength, root,
            loopStart, loopEnd,
            adsrEnabled, attack, decay, sustain, release,
            filterEnabled,
            filterCutoffCent, filterResonance, filterEnvAmount,
            filterAttack, filterDecay, filterSustain, filterRelease);

        auto ms = std::make_unique<Timbre::MappedSample>(s, lowerNoteNo, upperNoteNo, lowerVelocity, upperVelocity);
        samples->push_back(std::move(ms));
    }

    if (samples->empty()) {
        ESP_LOGW(LOG_TAG, "No samples found in: %s", path);
        return nullptr;
    }

    ESP_LOGI(LOG_TAG, "Loaded timbre: %s (%zu samples)", path, samples->size());
    return std::make_shared<Timbre>(samples.release());
}

std::vector<TimbreInfo> TimbreLoader::scanTimbres()
{
    std::vector<TimbreInfo> result;

    if (!isLittleFSMounted()) {
        ESP_LOGE(LOG_TAG, "LittleFS not mounted. Call mountLittleFS() first.");
        return result;
    }

    std::string scanRoot = std::string(LITTLEFS_MOUNT_POINT) + "/timbres";
    DIR* dir = opendir(scanRoot.c_str());
    if (!dir) {
        ESP_LOGW(LOG_TAG, "Failed to open directory: %s", scanRoot.c_str());
        return result;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // "." と ".." をスキップ
        if (entry->d_name[0] == '.') continue;

        std::string id = entry->d_name;
        std::string jsonRel = "/timbres/" + id + "/" + id + ".json";
        std::string jsonFull = std::string(LITTLEFS_MOUNT_POINT) + jsonRel;

        // ディレクトリかどうか確認（JSONを開いてみるだけでも代用可能）
        FILE* file = fopen(jsonFull.c_str(), "r");
        if (!file) {
            // <id>/<id>.json がなければスキップ
            continue;
        }

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        std::vector<char> buffer(fileSize + 1);
        size_t bytesRead = fread(buffer.data(), 1, fileSize, file);
        buffer[bytesRead] = '\0';
        fclose(file);

        // 軽量JSONパース。samples 配列はスキップしたいので
        // DeserializationOption::Filter を使う
        StaticJsonDocument<64> filter;
        filter["name"] = true;
        filter["category"] = true;

        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, buffer.data(),
            DeserializationOption::Filter(filter));
        if (error) {
            ESP_LOGW(LOG_TAG, "Failed to parse JSON: %s (%s)", jsonFull.c_str(), error.c_str());
            continue;
        }

        TimbreInfo info;
        info.id = id;
        info.jsonPath = jsonRel;
        info.name = doc["name"] | id.c_str();          // name がなければ id を流用
        info.category = doc["category"] | "";          // category 未指定はメイン音色から除外する想定

        result.push_back(std::move(info));
    }

    closedir(dir);
    ESP_LOGI(LOG_TAG, "Scanned %zu timbres in %s", result.size(), scanRoot.c_str());
    return result;
}

TimbreLoader Loader;
