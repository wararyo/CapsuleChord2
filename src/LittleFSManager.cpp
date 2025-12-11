#include "LittleFSManager.h"

static const char* LOG_TAG = "LittleFSManager";
static bool g_littlefs_mounted = false;

bool mountLittleFS()
{
    if (g_littlefs_mounted) {
        return true;
    }

    esp_vfs_littlefs_conf_t conf = {
        .base_path = LITTLEFS_MOUNT_POINT,
        .partition_label = "littlefs",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(LOG_TAG, "Failed to mount LittleFS partition");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(LOG_TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(LOG_TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(LOG_TAG, "LittleFS mounted. Total: %d bytes, Used: %d bytes", total, used);
    }

    g_littlefs_mounted = true;
    return true;
}

bool isLittleFSMounted()
{
    return g_littlefs_mounted;
}
