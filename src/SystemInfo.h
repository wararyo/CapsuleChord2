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

struct CrashInfo {
    bool available = false;            // 有効なコアダンプを読み出せた
    std::string taskName;              // クラッシュしたタスク名
    std::string panicReason;           // 人間可読の原因（取得できた場合）
    uint32_t pc = 0;                   // 例外発生時のプログラムカウンタ
    uint32_t excCause = 0;             // 例外原因コード（Xtensa）
    uint32_t excVaddr = 0;             // 不正アクセス先アドレス（該当する場合）
    uint32_t backtrace[16] = {};       // バックトレース（PCの配列）
    uint32_t backtraceDepth = 0;       // バックトレースの段数
    bool backtraceCorrupted = false;   // バックトレースが破損しているか
    std::string elfSha256;             // クラッシュしたファームのELF SHA256（addr2line用ELF照合）
};

FirmwareInfo getFirmwareInfo();
ChipInfoSummary getChipInfoSummary();
RuntimeInfo getRuntimeInfo();
FilesystemInfo getFilesystemInfo();
CrashInfo getCrashInfo();

std::string buildFirmwareInfoText();
std::string buildDiagnosticsText();
