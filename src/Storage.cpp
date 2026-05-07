#include "Storage.h"

#include <FS.h>
#include <M5Stack.h>
#include <SD.h>

#include <algorithm>

#include "Config.h"

namespace journal {

bool Storage::ready_ = false;

bool Storage::begin() {
  ready_ = SD.begin();
  if (ready_) ensureDir();
  return ready_;
}

bool Storage::ensureDir() {
  if (!SD.exists(JOURNAL_DIR)) {
    return SD.mkdir(JOURNAL_DIR);
  }
  return true;
}

String Storage::pathForDate(const char* yyyymmdd) {
  String p = JOURNAL_DIR;
  p += "/";
  p += yyyymmdd;
  p += ".txt";
  return p;
}

String Storage::undatedPath(uint32_t bootCount) {
  String p = JOURNAL_DIR;
  p += "/UNDATED-";
  p += String(bootCount);
  p += ".txt";
  return p;
}

bool Storage::readEntry(const char* path, String& out, size_t cap) {
  out = "";
  if (!ready_) return false;
  if (!SD.exists(path)) return true;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  size_t n = f.size();
  if (n > cap) n = cap;
  out.reserve(n + 1);
  while (out.length() < n && f.available()) {
    out += (char)f.read();
  }
  f.close();
  return true;
}

bool Storage::saveAtomic(const char* path, const char* data, size_t len) {
  if (!ready_) return false;
  if (!ensureDir()) return false;

  String tmp = String(path) + ".tmp";
  File f = SD.open(tmp.c_str(), FILE_WRITE);
  if (!f) return false;
  size_t written = 0;
  while (written < len) {
    size_t w = f.write((const uint8_t*)data + written, len - written);
    if (w == 0) { f.close(); return false; }
    written += w;
  }
  f.flush();
  f.close();

  if (SD.exists(path)) SD.remove(path);
  return SD.rename(tmp.c_str(), path);
}

bool Storage::listEntries(std::vector<String>& outNames) {
  outNames.clear();
  if (!ready_) return false;
  File dir = SD.open(JOURNAL_DIR);
  if (!dir || !dir.isDirectory()) return false;
  File entry;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String n = entry.name();
      int slash = n.lastIndexOf('/');
      if (slash >= 0) n = n.substring(slash + 1);
      if (n.endsWith(".txt") && !n.endsWith(".tmp")) {
        outNames.push_back(n);
      }
    }
    entry.close();
  }
  dir.close();
  std::sort(outNames.begin(), outNames.end(),
            [](const String& a, const String& b) { return a > b; });
  return true;
}

}  // namespace journal
