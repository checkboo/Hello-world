#pragma once

#include "Input.h"

namespace journal {

// M5Stack CardKB Unit (I2C QWERTY keyboard, address 0x5F on the GROVE A port).
// The unit returns one byte per Wire.requestFrom; 0x00 means "no key since
// last read". Printable ASCII passes through; arrow / control keys use a
// small set of high-bit codes; Sym + letter combos arrive as Ctrl-letter
// terminal codes (0x01..0x1A).
class CardKB {
 public:
  static bool begin();
  static bool present();
  static bool poll(InputKey& out);  // true if a key event is available
};

}  // namespace journal
