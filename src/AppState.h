#pragma once

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "BTKeyboard.h"
#include "Buttons.h"
#include "Editor.h"
#include "EntryList.h"
#include "Input.h"
#include "Viewer.h"

namespace journal {

enum class State : uint8_t {
  Boot,
  Pairing,
  WifiSync,
  Editor,
  EntryList,
  Viewer,
};

class App {
 public:
  void begin();
  void loop();

 private:
  State        state_      = State::Boot;
  Buttons      buttons_;
  Editor       editor_;
  Viewer       viewer_;
  EntryList    list_;
  QueueHandle_t keyQ_      = nullptr;
  bool         wifiOk_     = false;
  bool         hadKeyboard_ = false;

  // Status bar cache so we only repaint when something changes.
  uint32_t lastStatusMs_ = 0;
  String   dateCache_;

  void enter(State s);
  void onButton(const ButtonEvent& e);
  void onKey(const InputKey& e);
  void drawStatus(bool force = false);

  void doBoot();
  void doPairing();
  void doWifiSync();
  void enterEditorToday();

  static void btKeyTrampoline(const KeyEvent& e);
  static App* self_;
};

}  // namespace journal
