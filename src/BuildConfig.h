#pragma once

// Build-time feature switches. PlatformIO environments can override these
// macros in platformio.ini.
#ifndef CAPSULECHORD_ENABLE_DEV_FEATURES
#define CAPSULECHORD_ENABLE_DEV_FEATURES 0
#endif

#define CAPSULECHORD_INCLUDE_DEV_APPS CAPSULECHORD_ENABLE_DEV_FEATURES
#define CAPSULECHORD_SHOW_DEV_SETTINGS CAPSULECHORD_ENABLE_DEV_FEATURES
