#include "SystemInfo.h"

#include "Keypad.h"
#include "LittleFSManager.h"
#include "Output/MidiOutput.h"
#include "SettingsStore.h"

#include <esp_app_desc.h>
#include <esp_chip_info.h>
#include <esp_heap_caps.h>
#include <esp_idf_version.h>
#include <esp_littlefs.h>
#include <esp_system.h>
#include <esp_timer.h>

#include <cstdio>
#include <sstream>

FirmwareInfo getFirmwareInfo() {
    const esp_app_desc_t* app = esp_app_get_description();
    FirmwareInfo info;
    if (app) {
        info.projectName = app->project_name;
        info.version = app->version;
        info.idfVersion = app->idf_ver;
        info.buildDate = app->date;
        info.buildTime = app->time;
    }
    return info;
}

static const char* chipModelToString(esp_chip_model_t model) {
    switch (model) {
        case CHIP_ESP32: return "ESP32";
        case CHIP_ESP32S2: return "ESP32-S2";
        case CHIP_ESP32S3: return "ESP32-S3";
        case CHIP_ESP32C3: return "ESP32-C3";
        case CHIP_ESP32C2: return "ESP32-C2";
        case CHIP_ESP32C6: return "ESP32-C6";
        case CHIP_ESP32H2: return "ESP32-H2";
        default: return "Unknown";
    }
}

ChipInfoSummary getChipInfoSummary() {
    esp_chip_info_t chip;
    esp_chip_info(&chip);

    ChipInfoSummary info;
    info.model = chipModelToString(chip.model);
    info.revision = chip.revision;
    info.cores = chip.cores;
    return info;
}

RuntimeInfo getRuntimeInfo() {
    RuntimeInfo info;
    info.uptimeSeconds = static_cast<uint32_t>(esp_timer_get_time() / 1000000ULL);
    info.freeHeapBytes = esp_get_free_heap_size();
    info.minimumFreeHeapBytes = esp_get_minimum_free_heap_size();
    info.freePsramBytes = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    return info;
}

FilesystemInfo getFilesystemInfo() {
    FilesystemInfo info;
    info.mounted = isLittleFSMounted();
    if (!info.mounted) {
        return info;
    }

    esp_err_t ret = esp_littlefs_info("littlefs", &info.totalBytes, &info.usedBytes);
    info.infoAvailable = (ret == ESP_OK);
    return info;
}

const char* formatBytes(size_t bytes) {
    static char buffers[4][24];
    static uint8_t index = 0;
    char* buf = buffers[index++ % 4];

    if (bytes >= 1024 * 1024) {
        snprintf(buf, 24, "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    } else if (bytes >= 1024) {
        snprintf(buf, 24, "%.1f KB", static_cast<double>(bytes) / 1024.0);
    } else {
        snprintf(buf, 24, "%u B", static_cast<unsigned>(bytes));
    }
    return buf;
}

static const char* outputTypeToString(OutputType type) {
    switch (type) {
        case OutputType::Internal: return "Internal";
        case OutputType::BleMidi: return "Bluetooth MIDI";
        case OutputType::UsbMidi: return "USB MIDI";
        default: return "Unknown";
    }
}

static const char* keypadProtocolToString(KeypadProtocol protocol) {
    switch (protocol) {
        case KeypadProtocol::Legacy: return "Legacy";
        case KeypadProtocol::V3: return "V3";
        default: return "Unknown";
    }
}

static std::string formatUptime(uint32_t seconds) {
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds / 60) % 60;
    uint32_t secs = seconds % 60;

    char buf[24];
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours, minutes, secs);
    return buf;
}

std::string buildFirmwareInfoText() {
    FirmwareInfo fw = getFirmwareInfo();
    ChipInfoSummary chip = getChipInfoSummary();
    KeypadFirmwareInfo keypad = Keypad.getFirmwareInfo();

    std::ostringstream ss;
    ss << "Firmware\n";
    ss << "Name: " << (fw.projectName.empty() ? "Unknown" : fw.projectName) << "\n";
    ss << "Version: " << (fw.version.empty() ? "Unknown" : fw.version) << "\n";
    ss << "Build: " << fw.buildDate << " " << fw.buildTime << "\n";
    ss << "ESP-IDF: " << (fw.idfVersion.empty() ? esp_get_idf_version() : fw.idfVersion.c_str()) << "\n";
    ss << "\n";
    ss << "Device\n";
    ss << "Board: M5Stack CoreS3\n";
    ss << "Chip: " << chip.model << " rev " << static_cast<int>(chip.revision) << "\n";
    ss << "CPU cores: " << static_cast<int>(chip.cores) << "\n";
    ss << "\n";
    ss << "Keypad\n";
    ss << "I2C address: 0x09\n";
    ss << "Protocol: " << keypadProtocolToString(keypad.protocol) << "\n";
    ss << "Firmware: ";
    if (keypad.versionAvailable) {
        ss << "v" << static_cast<int>(keypad.major) << "." << static_cast<int>(keypad.minor) << "\n";
    } else {
        ss << "Unknown\n";
    }
    return ss.str();
}

std::string buildDiagnosticsText() {
    RuntimeInfo runtime = getRuntimeInfo();
    FilesystemInfo fs = getFilesystemInfo();
    KeypadFirmwareInfo keypad = Keypad.getFirmwareInfo();

    std::ostringstream ss;
    ss << "Runtime\n";
    ss << "Uptime: " << formatUptime(runtime.uptimeSeconds) << "\n";
    ss << "Free heap: " << formatBytes(runtime.freeHeapBytes) << "\n";
    ss << "Min free heap: " << formatBytes(runtime.minimumFreeHeapBytes) << "\n";
    ss << "Free PSRAM: " << formatBytes(runtime.freePsramBytes) << "\n";
    ss << "\n";
    ss << "Storage\n";
    ss << "LittleFS: " << (fs.mounted ? "Mounted" : "Not mounted") << "\n";
    if (fs.infoAvailable) {
        ss << "Used: " << formatBytes(fs.usedBytes) << " / " << formatBytes(fs.totalBytes) << "\n";
    }
    ss << "\n";
    ss << "Output\n";
    ss << "Current: " << outputTypeToString(Output.getCurrentOutputType()) << "\n";
    ss << "Setting: " << outputTypeToString(static_cast<OutputType>(Settings.output.outputTarget.get())) << "\n";
    ss << "Timbre: " << Settings.output.timbreId.get() << "\n";
    ss << "Speaker vol: " << static_cast<int>(Settings.output.speakerVolume.get()) << "\n";
    ss << "Headphone vol: " << static_cast<int>(Settings.output.headphoneVolume.get()) << "\n";
    ss << "\n";
    ss << "Display\n";
    ss << "Brightness: " << static_cast<int>(Settings.display.brightness.get()) << "\n";
    ss << "Key LED brightness: " << static_cast<int>(Settings.display.keypadBrightness.get()) << "\n";
    ss << "\n";
    ss << "Keypad\n";
    ss << "Protocol: " << keypadProtocolToString(keypad.protocol) << "\n";
    if (keypad.versionAvailable) {
        ss << "Firmware: v" << static_cast<int>(keypad.major) << "." << static_cast<int>(keypad.minor) << "\n";
    } else {
        ss << "Firmware: Unknown\n";
    }

    return ss.str();
}
