#pragma once

#include <Arduino.h>

#include "Ui.h"

namespace journal {

struct KeyEvent {
  uint8_t modifiers;
  uint8_t usage;     // HID Usage ID (page 0x07)
  bool    down;
};

struct DiscoveredPeer {
  uint8_t bda[6];
  char    name[32];
};

using KeyCallback = void (*)(const KeyEvent&);

class BTKeyboard {
 public:
  static bool begin(KeyCallback cb);

  // Try to connect to the bonded peer (if one is stored). Returns true if a
  // connection attempt was issued; the actual outcome is delivered via state().
  static bool connectBonded();

  // Pairing UI helpers — run a blocking scan & let the caller pick a peer.
  static bool startScan();
  static void stopScan();
  static size_t discoveredCount();
  static const DiscoveredPeer& discovered(size_t i);

  static bool pairAndBond(const DiscoveredPeer& peer);
  static void forgetBond();
  static bool hasBond();

  static BtState state();
};

}  // namespace journal
