#include "Buttons.h"

#include <M5Stack.h>

namespace journal {

namespace {
bool isDown(Btn b) {
  switch (b) {
    case Btn::A: return M5.BtnA.isPressed();
    case Btn::B: return M5.BtnB.isPressed();
    case Btn::C: return M5.BtnC.isPressed();
  }
  return false;
}
}  // namespace

void Buttons::captureBootState() {
  M5.update();
  for (int i = 0; i < 3; ++i) {
    heldAtBoot_[i] = isDown((Btn)i);
    wasDown_[i]    = heldAtBoot_[i];
    if (heldAtBoot_[i]) pressedAt_[i] = millis();
  }
}

bool Buttons::push(ButtonEvent e) {
  uint8_t next = (qHead_ + 1) % 8;
  if (next == qTail_) return false;
  queue_[qHead_] = e;
  qHead_ = next;
  return true;
}

void Buttons::update() {
  uint32_t now = millis();
  for (int i = 0; i < 3; ++i) {
    bool down = isDown((Btn)i);
    if (down && !wasDown_[i]) {
      pressedAt_[i] = now;
      longFired_[i] = false;
    } else if (down && wasDown_[i]) {
      if (!longFired_[i] && now - pressedAt_[i] >= LONG_PRESS_MS) {
        longFired_[i] = true;
        push({(Btn)i, true});
      }
    } else if (!down && wasDown_[i]) {
      if (!longFired_[i]) push({(Btn)i, false});
    }
    wasDown_[i] = down;
  }
}

bool Buttons::poll(ButtonEvent& out) {
  if (qHead_ == qTail_) return false;
  out = queue_[qTail_];
  qTail_ = (qTail_ + 1) % 8;
  return true;
}

}  // namespace journal
