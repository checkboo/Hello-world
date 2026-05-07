#pragma once

#include <stdint.h>

namespace journal {

// Canonical key codes consumed by Editor / AppState.
// Positive values are printable ASCII; negative values are named control keys.
enum : int {
  KEY_NONE      = 0,
  KEY_BACKSPACE = -1,
  KEY_ENTER     = -2,
  KEY_TAB       = -3,
  KEY_ESC       = -4,
  KEY_LEFT      = -5,
  KEY_RIGHT     = -6,
  KEY_UP        = -7,
  KEY_DOWN      = -8,
  KEY_HOME      = -9,
  KEY_END       = -10,
  KEY_PAGEUP    = -11,
  KEY_PAGEDOWN  = -12,
  KEY_DELETE    = -13,
};

struct InputKey {
  int  code;
  bool ctrl;
  bool down;
};

}  // namespace journal
