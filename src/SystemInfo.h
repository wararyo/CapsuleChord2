#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct FirmwareInfo {
    std::string projectName;
    std::string version;
    std::string idfVersion;
    std::string buildDate;
    std::string buildTime;
};

struct ChipInfoSummary {
    std::string model;
    uint8_t revision = 0;
    uint8_t cores = 0;
};

struct RuntimeInfo {
    uint32_t uptimeSeconds = 0;
    uint32_t freeHeapBytes = 0;
    uint32_t minimumFreeHeapBytes = 0;
    uint32_t freePsramBytes = 0;
};

struct FilesystemInfo {
    bool mounted = false;
    bool infoAvailable = false;
    size_t totalBytes = 0;
    size_t usedBytes = 0;
};

FirmwareInfo getFirmwareInfo();
ChipInfoSummary getChipInfoSummary();
RuntimeInfo getRuntimeInfo();
FilesystemInfo getFilesystemInfo();

const char* formatBytes(size_t bytes);
std::string buildFirmwareInfoText();
std::string buildDiagnosticsText();
