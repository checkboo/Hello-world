#include "hid_keymap.h"

namespace journal {

int hidUsageToKey(uint8_t usage, uint8_t mod) {
  const bool shift = isShift(mod);

  // Letters: HID 0x04..0x1D = 'a'..'z'
  if (usage >= 0x04 && usage <= 0x1D) {
    char c = 'a' + (usage - 0x04);
    return shift ? (c - 32) : c;
  }
  // Numbers row: 0x1E..0x27 = '1' '2' ... '9' '0'
  if (usage >= 0x1E && usage <= 0x27) {
    static const char unshifted[10] = {'1','2','3','4','5','6','7','8','9','0'};
    static const char shifted[10]   = {'!','@','#','$','%','^','&','*','(',')'};
    return shift ? shifted[usage - 0x1E] : unshifted[usage - 0x1E];
  }

  switch (usage) {
    case 0x28: return KEY_ENTER;
    case 0x29: return KEY_ESC;
    case 0x2A: return KEY_BACKSPACE;
    case 0x2B: return KEY_TAB;
    case 0x2C: return ' ';
    case 0x2D: return shift ? '_' : '-';
    case 0x2E: return shift ? '+' : '=';
    case 0x2F: return shift ? '{' : '[';
    case 0x30: return shift ? '}' : ']';
    case 0x31: return shift ? '|' : '\\';
    case 0x33: return shift ? ':' : ';';
    case 0x34: return shift ? '"' : '\'';
    case 0x35: return shift ? '~' : '`';
    case 0x36: return shift ? '<' : ',';
    case 0x37: return shift ? '>' : '.';
    case 0x38: return shift ? '?' : '/';
    case 0x4A: return KEY_HOME;
    case 0x4B: return KEY_PAGEUP;
    case 0x4C: return KEY_DELETE;
    case 0x4D: return KEY_END;
    case 0x4E: return KEY_PAGEDOWN;
    case 0x4F: return KEY_RIGHT;
    case 0x50: return KEY_LEFT;
    case 0x51: return KEY_DOWN;
    case 0x52: return KEY_UP;
    default:   return KEY_NONE;
  }
}

}  // namespace journal
