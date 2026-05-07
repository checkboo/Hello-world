#pragma once

#include <stdint.h>

namespace journal {

constexpr const char* JOURNAL_DIR     = "/journal";
constexpr const char* WIFI_FILE       = "/wifi.txt";
constexpr const char* DEFAULT_TZ      = "UTC0";

constexpr uint32_t AUTOSAVE_MS        = 2000;
constexpr uint32_t WIFI_TIMEOUT_MS    = 8000;
constexpr uint32_t NTP_TIMEOUT_MS     = 6000;
constexpr uint32_t PAIRING_SCAN_S     = 20;

constexpr size_t   MAX_ENTRY_BYTES    = 256 * 1024;

constexpr uint16_t COLOR_BG           = 0x0000;
constexpr uint16_t COLOR_FG           = 0xFFFF;
constexpr uint16_t COLOR_DIM          = 0x7BEF;
constexpr uint16_t COLOR_ACCENT       = 0x07FF;
constexpr uint16_t COLOR_WARN         = 0xFD20;
constexpr uint16_t COLOR_ERR          = 0xF800;
constexpr uint16_t COLOR_OK           = 0x07E0;
constexpr uint16_t COLOR_STATUSBAR_BG = 0x2104;

constexpr int16_t  SCREEN_W           = 320;
constexpr int16_t  SCREEN_H           = 240;
constexpr int16_t  STATUSBAR_H        = 16;
constexpr int16_t  CHAR_W             = 6;
constexpr int16_t  LINE_H             = 13;

constexpr const char* NVS_NS          = "journal";
constexpr const char* NVS_KEY_EPOCH   = "epoch";
constexpr const char* NVS_KEY_BOOTCNT = "bootcnt";
constexpr const char* NVS_KEY_BONDED  = "bonded";

}  // namespace journal
