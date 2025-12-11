#pragma once

#include <esp_littlefs.h>
#include <esp_log.h>

// LittleFSマウントポイント
#define LITTLEFS_MOUNT_POINT "/littlefs"

/**
 * @brief LittleFSをマウントする（グローバル関数）
 *
 * 起動時に一度だけ呼び出す。すでにマウント済みの場合は何もしない。
 * @return 成功した場合true
 */
bool mountLittleFS();

/**
 * @brief LittleFSがマウント済みかどうかを返す
 * @return マウント済みの場合true
 */
bool isLittleFSMounted();
