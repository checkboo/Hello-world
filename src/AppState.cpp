#include "AppState.h"

#include <M5Stack.h>

#include "CardKB.h"
#include "Config.h"
#include "Storage.h"
#include "TimeSync.h"
#include "Ui.h"
#include "WifiCreds.h"
#include "hid_keymap.h"

namespace journal {

App* App::self_ = nullptr;

void App::btKeyTrampoline(const KeyEvent& e) {
  if (!self_ || !self_->keyQ_) return;
  // BT callback runs in the HIDH event task — translate to canonical InputKey
  // here so the editor doesn't have to know about HID, and push to the queue.
  InputKey ik;
  ik.code = hidUsageToKey(e.usage, e.modifiers);
  ik.ctrl = isCtrl(e.modifiers);
  ik.down = e.down;
  xQueueSend(self_->keyQ_, &ik, 0);
}

void App::begin() {
  self_ = this;
  M5.begin(true /*lcd*/, true /*sd*/, true /*serial*/, false /*i2c*/);
  M5.Power.begin();
  Ui::begin();
  buttons_.captureBootState();

  keyQ_ = xQueueCreate(64, sizeof(InputKey));
  CardKB::begin();  // probe the GROVE port; OK if not present.

  Ui::splash("Journal", "booting...");
  delay(300);
  doBoot();
}

void App::doBoot() {
  if (!Storage::begin()) {
    Ui::error("No SD card", "Insert an SD card and reboot.");
    while (true) { delay(1000); }
  }

  // Hold B at boot → pairing flow.
  if (buttons_.heldAtBoot(Btn::B)) {
    enter(State::Pairing);
    return;
  }
  enter(State::WifiSync);
}

void App::doWifiSync() {
  Ui::splash("Journal", "syncing time...");
  WifiCreds creds;
  loadWifiCreds(creds);
  wifiOk_ = TimeSync::beginAndSync(
      creds.valid ? creds.ssid.c_str() : nullptr,
      creds.valid ? creds.pass.c_str() : nullptr,
      creds.valid ? creds.tz.c_str()   : DEFAULT_TZ);

  // Bring up BT keyboard (best-effort; editor still usable without it).
  BTKeyboard::begin(&App::btKeyTrampoline);
  if (BTKeyboard::hasBond()) BTKeyboard::connectBonded();

  enterEditorToday();
}

void App::doPairing() {
  Ui::splash("Pairing", "scanning BLE keyboards...");
  BTKeyboard::begin(&App::btKeyTrampoline);
  BTKeyboard::forgetBond();
  bool any = BTKeyboard::startScan();
  if (!any) {
    Ui::error("No BLE keyboards", "Put your keyboard in pairing mode and reboot.");
    delay(2500);
    enter(State::WifiSync);
    return;
  }

  // Simple picker: A = up, C = down, B = select, long-A = cancel.
  size_t sel = 0;
  auto draw = [&]() {
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(COLOR_FG, COLOR_BG);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print(" Select keyboard  (A up / C down / B ok)");
    size_t visible = (size_t)Ui::rows();
    size_t scroll  = (sel < visible) ? 0 : sel - visible + 1;
    for (size_t row = 0; row < visible; ++row) {
      size_t i = scroll + row;
      if (i >= BTKeyboard::discoveredCount()) break;
      int16_t y = Ui::bodyTop() + (int16_t)row * LINE_H;
      bool isSel = (i == sel);
      if (isSel) M5.Lcd.fillRect(0, y, SCREEN_W, LINE_H, COLOR_ACCENT);
      M5.Lcd.setTextColor(isSel ? COLOR_BG : COLOR_FG, isSel ? COLOR_ACCENT : COLOR_BG);
      M5.Lcd.setCursor(8, y);
      const auto& p = BTKeyboard::discovered(i);
      M5.Lcd.print(p.name[0] ? p.name : "(unnamed)");
    }
  };
  draw();
  while (true) {
    M5.update();
    buttons_.update();
    ButtonEvent be;
    while (buttons_.poll(be)) {
      if (be.btn == Btn::A && be.longPress) { enter(State::WifiSync); return; }
      if (be.btn == Btn::A && sel > 0)                                 { --sel; draw(); }
      if (be.btn == Btn::C && sel + 1 < BTKeyboard::discoveredCount()) { ++sel; draw(); }
      if (be.btn == Btn::B) {
        Ui::splash("Pairing", "connecting...");
        if (BTKeyboard::pairAndBond(BTKeyboard::discovered(sel))) {
          Ui::toast("Paired", 600);
        } else {
          Ui::toast("Failed", 800);
        }
        enter(State::WifiSync);
        return;
      }
    }
    delay(20);
  }
}

void App::enterEditorToday() {
  String path;
  if (TimeSync::isValid()) {
    path = Storage::pathForDate(TimeSync::today().c_str());
  } else {
    path = Storage::undatedPath(TimeSync::bootCount());
  }
  String body;
  Storage::readEntry(path.c_str(), body, MAX_ENTRY_BYTES);
  editor_.open(path.c_str(), body);
  enter(State::Editor);
}

void App::enter(State s) {
  state_ = s;
  switch (s) {
    case State::Boot:                                   break;
    case State::Pairing:    doPairing();                break;
    case State::WifiSync:   doWifiSync();               break;
    case State::Editor:
      Ui::clearBody();
      editor_.render();
      drawStatus(true);
      break;
    case State::EntryList:
      list_.refresh();
      Ui::clearBody();
      list_.render();
      drawStatus(true);
      break;
    case State::Viewer:
      Ui::clearBody();
      viewer_.render();
      drawStatus(true);
      break;
  }
}

void App::onButton(const ButtonEvent& e) {
  switch (state_) {
    case State::Editor:
      if (e.btn == Btn::A) {
        editor_.saveNow();
        Ui::toast("Saved", 400);
        editor_.render();
        drawStatus(true);
      } else if (e.btn == Btn::C) {
        editor_.saveNow();
        enter(State::EntryList);
      }
      break;
    case State::EntryList:
      if (e.btn == Btn::A)      { list_.moveUp();   list_.render(); }
      else if (e.btn == Btn::C) { list_.moveDown(); list_.render(); }
      else if (e.btn == Btn::B) {
        if (!list_.empty()) {
          if (viewer_.open(list_.selectedPath().c_str())) {
            enter(State::Viewer);
          }
        } else {
          enter(State::Editor);
        }
      }
      break;
    case State::Viewer:
      if (e.btn == Btn::A)      { viewer_.scrollUp();   viewer_.render(); }
      else if (e.btn == Btn::C) { viewer_.scrollDown(); viewer_.render(); }
      else if (e.btn == Btn::B) { enter(State::EntryList); }
      break;
    default: break;
  }
}

void App::onKey(const InputKey& e) {
  hadKeyboard_ = true;
  if (state_ == State::Editor) {
    if (e.down && e.code == KEY_ESC) {
      editor_.saveNow();
      enter(State::EntryList);
      return;
    }
    editor_.onKey(e);
    if (e.down) {
      editor_.render();
      drawStatus();
    }
  } else if (state_ == State::EntryList && e.down) {
    if (e.code == KEY_UP)        { list_.moveUp();   list_.render(); }
    else if (e.code == KEY_DOWN) { list_.moveDown(); list_.render(); }
    else if (e.code == KEY_ENTER && !list_.empty()) {
      if (viewer_.open(list_.selectedPath().c_str())) enter(State::Viewer);
    } else if (e.code == KEY_ESC) {
      enter(State::Editor);
    }
  } else if (state_ == State::Viewer && e.down) {
    if (e.code == KEY_UP || e.code == KEY_PAGEUP)         { viewer_.scrollUp();   viewer_.render(); }
    else if (e.code == KEY_DOWN || e.code == KEY_PAGEDOWN){ viewer_.scrollDown(); viewer_.render(); }
    else if (e.code == KEY_ESC || e.code == KEY_BACKSPACE){ enter(State::EntryList); }
  }
}

void App::drawStatus(bool force) {
  uint32_t now = millis();
  if (!force && now - lastStatusMs_ < 250) return;
  lastStatusMs_ = now;
  StatusInfo s{};
  String d = TimeSync::isValid() ? TimeSync::today() : String("TIME NOT SET");
  dateCache_ = d;
  s.dateStr   = dateCache_.c_str();
  s.timeValid = TimeSync::isValid();
  s.bt        = BTKeyboard::state();
  s.wifi      = wifiOk_;
  s.sd        = Storage::ready();
  s.dirty     = (state_ == State::Editor) && editor_.dirty();
  Ui::drawStatusBar(s);
}

void App::loop() {
  M5.update();
  buttons_.update();

  ButtonEvent be;
  while (buttons_.poll(be)) onButton(be);

  InputKey ckb;
  if (CardKB::poll(ckb) && keyQ_) xQueueSend(keyQ_, &ckb, 0);

  InputKey ik;
  while (keyQ_ && xQueueReceive(keyQ_, &ik, 0) == pdTRUE) onKey(ik);

  if (state_ == State::Editor) {
    bool wasDirty = editor_.dirty();
    editor_.tick();
    if (wasDirty && !editor_.dirty()) drawStatus(true);
  }

  drawStatus();
  delay(5);
}

}  // namespace journal
