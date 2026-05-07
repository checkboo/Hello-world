#pragma once

#include <Arduino.h>

#include <vector>

namespace journal {

class Storage {
 public:
  static bool begin();
  static bool ready() { return ready_; }

  static String pathForDate(const char* yyyymmdd);
  static String undatedPath(uint32_t bootCount);

  static bool readEntry(const char* path, String& out, size_t cap);
  static bool saveAtomic(const char* path, const char* data, size_t len);

  static bool listEntries(std::vector<String>& outNames);

 private:
  static bool ready_;
  static bool ensureDir();
};

}  // namespace journal
