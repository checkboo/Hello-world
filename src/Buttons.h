#pragma once

#include <stdint.h>

namespace journal {

enum class Btn : uint8_t { A = 0, B = 1, C = 2 };

struct ButtonEvent {
  Btn      btn;
  bool     longPress;
};

class Buttons {
 public:
  void update();
  bool poll(ButtonEvent& out);
  bool heldAtBoot(Btn b) const { return heldAtBoot_[(int)b]; }
  void captureBootState();

 private:
  static constexpr uint32_t LONG_PRESS_MS = 600;
  uint32_t pressedAt_[3] = {0, 0, 0};
  bool     wasDown_[3]   = {false, false, false};
  bool     longFired_[3] = {false, false, false};
  bool     heldAtBoot_[3] = {false, false, false};

  ButtonEvent queue_[8];
  uint8_t qHead_ = 0;
  uint8_t qTail_ = 0;
  bool push(ButtonEvent e);
};

}  // namespace journal
