#pragma once

#include <stdint.h>

#include "Input.h"

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

// US layout. Returns positive for printable ASCII, negative for KEY_* named
// keys (defined in Input.h), or KEY_NONE if unmapped.
int hidUsageToKey(uint8_t usage, uint8_t modifiers);

}  // namespace journal
