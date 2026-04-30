#pragma once

#include <Arduino.h>

namespace journal {

struct WifiCreds {
  String ssid;
  String pass;
  String tz;
  bool   valid = false;
};

bool loadWifiCreds(WifiCreds& out);

}  // namespace journal
