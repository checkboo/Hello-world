#include "TimeSync.h"

#include <Preferences.h>
#include <WiFi.h>
#include <sys/time.h>

#include "Config.h"

namespace journal {

bool     TimeSync::valid_ = false;
uint32_t TimeSync::bootCount_ = 0;

void TimeSync::loadCachedEpoch() {
  Preferences p;
  if (!p.begin(NVS_NS, true)) return;
  uint64_t epoch = p.getULong64(NVS_KEY_EPOCH, 0);
  p.end();
  if (epoch > 1700000000ULL) {
    timeval tv{(time_t)epoch, 0};
    settimeofday(&tv, nullptr);
  }
}

void TimeSync::cacheEpoch() {
  time_t now = time(nullptr);
  if (now < 1700000000) return;
  Preferences p;
  if (!p.begin(NVS_NS, false)) return;
  p.putULong64(NVS_KEY_EPOCH, (uint64_t)now);
  p.end();
}

void TimeSync::bumpBootCount() {
  Preferences p;
  if (!p.begin(NVS_NS, false)) return;
  bootCount_ = p.getUInt(NVS_KEY_BOOTCNT, 0) + 1;
  p.putUInt(NVS_KEY_BOOTCNT, bootCount_);
  p.end();
}

bool TimeSync::beginAndSync(const char* ssid, const char* pass, const char* tz) {
  bumpBootCount();
  loadCachedEpoch();

  if (!ssid || !*ssid) {
    valid_ = time(nullptr) > 1700000000;
    return valid_;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_OFF);
    valid_ = time(nullptr) > 1700000000;
    return valid_;
  }

  configTzTime(tz && *tz ? tz : DEFAULT_TZ, "pool.ntp.org", "time.nist.gov");
  uint32_t t0 = millis();
  while (time(nullptr) < 1700000000 && millis() - t0 < NTP_TIMEOUT_MS) {
    delay(100);
  }

  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);

  valid_ = time(nullptr) > 1700000000;
  if (valid_) cacheEpoch();
  return valid_;
}

bool TimeSync::isValid() { return valid_; }

String TimeSync::today() {
  time_t t = time(nullptr);
  struct tm tm;
  localtime_r(&t, &tm);
  char buf[16];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
  return String(buf);
}

String TimeSync::now() {
  time_t t = time(nullptr);
  struct tm tm;
  localtime_r(&t, &tm);
  char buf[12];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
  return String(buf);
}

uint32_t TimeSync::bootCount() { return bootCount_; }

}  // namespace journal
