#include <vector>
#include <string>
#include <ArduinoJson.h>
#include <cassert>
#include "TimbreLoader.h"

WavFile WavFile::open(fs::FS &fs, const char *path)
{
    if (!fs.exists(path))
    {
        Serial.println("File does not exist");
        return WavFile();
    }
    File file = fs.open(path);
    wav_header_t header;
    size_t bytes_read = file.read((uint8_t *)&header, sizeof(wav_header_t));
    if (bytes_read != sizeof(wav_header_t))
    {
        Serial.println("Failed to read header");
        file.close();
        return WavFile();
    }
    if (strncmp((const char *)header.Format, "WAVE", 4) != 0)
    {
        Serial.println("Invalid format");
        file.close();
        return WavFile();
    }
    
    wav_subchunk_header_t subchunkHeader;
    while(true){
        bytes_read = file.read((uint8_t *)&subchunkHeader, sizeof(wav_subchunk_header_t));
        if (bytes_read != sizeof(wav_subchunk_header_t))
        {
            Serial.println("Failed to read subchunk header");
            file.close();
            return WavFile();
        }
        if (strncmp((const char *)subchunkHeader.SubchunkID, "data", 4) == 0)
        {
            break;
        }
        file.seek(file.position() + subchunkHeader.SubchunkSize);
    }
    return WavFile(file, header, subchunkHeader);
}

void WavFile::close()
{
    file.close();
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
    return file.read((uint8_t *)data, size);
}

std::shared_ptr<Timbre> TimbreLoader::loadTimbre(fs::FS &fs, const char *path)
{
    if (!fs.exists(path))
    {
        Serial.println("File does not exist");
        return nullptr;
    }
    File file = fs.open(path);
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        Serial.print("Failed to read file, error: ");
        Serial.println(error.c_str());
        file.close();
        return nullptr;
    }

    std::string fullPath(path);
    std::string directoryPath = fullPath.substr(0, fullPath.find_last_of('/'));

    // Use unique_ptr to ensure proper cleanup on all code paths
    auto samples = std::make_unique<std::vector<std::unique_ptr<Timbre::MappedSample>>>();

    JsonArray samplesJson = doc["samples"].as<JsonArray>();
    for (JsonVariant sampleJson : samplesJson)
    {
        const uint8_t lowerNoteNo = sampleJson["lower-note-no"];
        const uint8_t upperNoteNo = sampleJson["upper-note-no"];
        const uint8_t lowerVelocity = sampleJson["lower-velocity"];
        const uint8_t upperVelocity = sampleJson["upper-velocity"];
        const char *samplePath = sampleJson["sample"]["path"];
        const uint8_t root = sampleJson["sample"]["root"];
        const uint32_t loopStart = sampleJson["sample"]["loop-start"];
        const uint32_t loopEnd = sampleJson["sample"]["loop-end"];
        const bool adsrEnabled = sampleJson["sample"]["adsr-enabled"];
        const float attach = sampleJson["sample"]["attach"];
        const float decay = sampleJson["sample"]["decay"];
        const float sustain = sampleJson["sample"]["sustain"];
        const float release = sampleJson["sample"]["release"];

        WavFile wavFile = WavFile::open(fs, (directoryPath + "/" + samplePath).c_str());
        if (!wavFile.isValid())
        {
            Serial.println("Failed to open wav file");
            continue;
        }
        size_t dataSize = wavFile.getDataSize();
        int16_t *data = (int16_t *)heap_caps_malloc(dataSize, MALLOC_CAP_SPIRAM);
        size_t written_bytes = wavFile.read(data, dataSize);
        assert(written_bytes == dataSize);
        size_t sampleLength = wavFile.getSampleLength();
        std::shared_ptr<Sample> s = std::make_shared<Sample>(
            data, sampleLength, root,
            loopStart, loopEnd,
            adsrEnabled, attach, decay, sustain, release);
        auto ms = std::make_unique<Timbre::MappedSample>(s, lowerNoteNo, upperNoteNo, lowerVelocity, upperVelocity);
        samples->push_back(std::move(ms));
        wavFile.close();
    }
    if (samples->empty())
    {
        Serial.println("No samples found");
        return nullptr;
    }

    file.close();
    return std::make_shared<Timbre>(samples.release());
}

TimbreLoader Loader;
