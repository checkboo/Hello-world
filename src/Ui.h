#pragma once

#include <stdint.h>

namespace journal {

enum class BtState : uint8_t { Off, Scanning, Connecting, Connected, Lost };

struct StatusInfo {
  const char* dateStr;
  bool        timeValid;
  BtState     bt;
  bool        wifi;
  bool        sd;
  bool        dirty;
};

class Ui {
 public:
  static void begin();
  static void clearBody();
  static void drawStatusBar(const StatusInfo& s);
  static void splash(const char* line1, const char* line2 = nullptr);
  static void error(const char* line1, const char* line2 = nullptr);
  static void toast(const char* msg, uint32_t holdMs = 700);

  static int16_t bodyTop();
  static int16_t bodyHeight();
  static int16_t cols();
  static int16_t rows();
};

}  // namespace journal
