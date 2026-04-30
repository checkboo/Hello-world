#pragma once

#include <stdint.h>

namespace journal {

// HID keyboard modifier bits (USB HID spec).
constexpr uint8_t HID_MOD_LCTRL  = 0x01;
constexpr uint8_t HID_MOD_LSHIFT = 0x02;
constexpr uint8_t HID_MOD_LALT   = 0x04;
constexpr uint8_t HID_MOD_LGUI   = 0x08;
constexpr uint8_t HID_MOD_RCTRL  = 0x10;
constexpr uint8_t HID_MOD_RSHIFT = 0x20;
constexpr uint8_t HID_MOD_RALT   = 0x40;
constexpr uint8_t HID_MOD_RGUI   = 0x80;

constexpr bool isShift(uint8_t m) { return m & (HID_MOD_LSHIFT | HID_MOD_RSHIFT); }
constexpr bool isCtrl (uint8_t m) { return m & (HID_MOD_LCTRL  | HID_MOD_RCTRL ); }

// Special key codes returned by hidUsageToKey() above the printable ASCII range.
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

// US layout. Returns positive for printable ASCII, negative for named keys, 0 if unmapped.
int hidUsageToKey(uint8_t usage, uint8_t modifiers);

}  // namespace journal
