#pragma once

#include <Arduino.h>
#include <time.h>

namespace journal {

class TimeSync {
 public:
  static bool   beginAndSync(const char* ssid, const char* pass, const char* tz);
  static bool   isValid();
  static String today();           // "YYYY-MM-DD"
  static String now();             // "HH:MM:SS"
  static uint32_t bootCount();

 private:
  static bool       valid_;
  static uint32_t   bootCount_;
  static void  loadCachedEpoch();
  static void  cacheEpoch();
  static void  bumpBootCount();
};

}  // namespace journal
