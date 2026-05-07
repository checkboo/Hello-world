#include "CardKB.h"

#include <Wire.h>

namespace journal {

namespace {
constexpr uint8_t CARDKB_ADDR = 0x5F;
bool g_present = false;

int translate(uint8_t b, bool& ctrl) {
  ctrl = false;
  switch (b) {
    case 0x00: return KEY_NONE;
    case 0x08: return KEY_BACKSPACE;
    case 0x09: return KEY_TAB;
    case 0x0D: return KEY_ENTER;
    case 0x1B: return KEY_ESC;
    case 0xB4: return KEY_LEFT;
    case 0xB5: return KEY_UP;
    case 0xB6: return KEY_DOWN;
    case 0xB7: return KEY_RIGHT;
    default:
      if (b >= 0x20 && b <= 0x7E) return (int)b;
      // Sym + letter on CardKB arrives as terminal Ctrl codes 0x01..0x1A.
      if (b >= 0x01 && b <= 0x1A) {
        ctrl = true;
        return 'a' + (b - 0x01);
      }
      return KEY_NONE;
  }
}
}  // namespace

bool CardKB::begin() {
  Wire.begin();        // M5Stack Fire: SDA=21, SCL=22 by default.
  Wire.setClock(100000);
  Wire.beginTransmission(CARDKB_ADDR);
  uint8_t err = Wire.endTransmission();
  g_present = (err == 0);
  return g_present;
}

bool CardKB::present() { return g_present; }

bool CardKB::poll(InputKey& out) {
  if (!g_present) return false;
  uint8_t got = Wire.requestFrom((uint8_t)CARDKB_ADDR, (uint8_t)1);
  if (got != 1 || !Wire.available()) return false;
  uint8_t b = Wire.read();
  if (b == 0) return false;

  bool ctrl;
  int code = translate(b, ctrl);
  if (code == KEY_NONE) return false;
  out.code = code;
  out.ctrl = ctrl;
  out.down = true;
  return true;
}

}  // namespace journal
