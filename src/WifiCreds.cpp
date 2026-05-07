#include "WifiCreds.h"

#include <SD.h>

#include "Config.h"

namespace journal {

namespace {
String trim(const String& s) {
  int a = 0, b = s.length();
  while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
  while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
  return s.substring(a, b);
}
}  // namespace

bool loadWifiCreds(WifiCreds& out) {
  out = {};
  if (!SD.exists(WIFI_FILE)) return false;
  File f = SD.open(WIFI_FILE, FILE_READ);
  if (!f) return false;

  String line;
  while (f.available()) {
    char c = (char)f.read();
    if (c == '\n' || !f.available()) {
      if (c != '\n' && c != '\r') line += c;
      String t = trim(line);
      if (t.length() && t[0] != '#') {
        int eq = t.indexOf('=');
        if (eq > 0) {
          String k = trim(t.substring(0, eq));
          String v = trim(t.substring(eq + 1));
          if (k == "ssid") out.ssid = v;
          else if (k == "pass") out.pass = v;
          else if (k == "tz")   out.tz   = v;
        }
      }
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
  f.close();
  out.valid = out.ssid.length() > 0;
  if (out.tz.length() == 0) out.tz = DEFAULT_TZ;
  return out.valid;
}

}  // namespace journal
