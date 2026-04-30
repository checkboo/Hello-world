#include "Ui.h"

#include <M5Stack.h>

#include "Config.h"

namespace journal {

void Ui::begin() {
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(COLOR_BG);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(COLOR_FG, COLOR_BG);
}

void Ui::clearBody() {
  M5.Lcd.fillRect(0, STATUSBAR_H, SCREEN_W, SCREEN_H - STATUSBAR_H, COLOR_BG);
}

int16_t Ui::bodyTop()    { return STATUSBAR_H; }
int16_t Ui::bodyHeight() { return SCREEN_H - STATUSBAR_H; }
int16_t Ui::cols()       { return SCREEN_W / CHAR_W; }
int16_t Ui::rows()       { return bodyHeight() / LINE_H; }

void Ui::drawStatusBar(const StatusInfo& s) {
  M5.Lcd.fillRect(0, 0, SCREEN_W, STATUSBAR_H, COLOR_STATUSBAR_BG);
  M5.Lcd.setTextColor(s.timeValid ? COLOR_FG : COLOR_WARN, COLOR_STATUSBAR_BG);
  M5.Lcd.setCursor(4, 4);
  M5.Lcd.print(s.dateStr ? s.dateStr : "----");

  int16_t x = SCREEN_W - 4;
  auto drawTag = [&](const char* tag, uint16_t color) {
    int16_t w = (int16_t)strlen(tag) * CHAR_W;
    x -= w + 4;
    M5.Lcd.setTextColor(color, COLOR_STATUSBAR_BG);
    M5.Lcd.setCursor(x, 4);
    M5.Lcd.print(tag);
  };

  if (s.dirty) drawTag("*", COLOR_WARN);
  drawTag(s.sd ? "SD" : "sd", s.sd ? COLOR_OK : COLOR_ERR);
  drawTag(s.wifi ? "W" : "w", s.wifi ? COLOR_OK : COLOR_DIM);
  const char* btTag = "?";
  uint16_t btColor = COLOR_DIM;
  switch (s.bt) {
    case BtState::Off:        btTag = "bt"; btColor = COLOR_DIM;  break;
    case BtState::Scanning:   btTag = "BT"; btColor = COLOR_ACCENT; break;
    case BtState::Connecting: btTag = "BT"; btColor = COLOR_WARN; break;
    case BtState::Connected:  btTag = "BT"; btColor = COLOR_OK;   break;
    case BtState::Lost:       btTag = "BT"; btColor = COLOR_ERR;  break;
  }
  drawTag(btTag, btColor);
}

void Ui::splash(const char* line1, const char* line2) {
  M5.Lcd.fillScreen(COLOR_BG);
  M5.Lcd.setTextColor(COLOR_FG, COLOR_BG);
  M5.Lcd.setTextSize(2);
  int16_t y = 80;
  if (line1) {
    M5.Lcd.setCursor((SCREEN_W - (int16_t)strlen(line1) * 12) / 2, y);
    M5.Lcd.print(line1);
    y += 28;
  }
  M5.Lcd.setTextSize(1);
  if (line2) {
    M5.Lcd.setCursor((SCREEN_W - (int16_t)strlen(line2) * CHAR_W) / 2, y);
    M5.Lcd.print(line2);
  }
}

void Ui::error(const char* line1, const char* line2) {
  M5.Lcd.fillScreen(COLOR_BG);
  M5.Lcd.setTextColor(COLOR_ERR, COLOR_BG);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 80);
  M5.Lcd.print(line1 ? line1 : "ERROR");
  if (line2) {
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(COLOR_FG, COLOR_BG);
    M5.Lcd.setCursor(20, 120);
    M5.Lcd.print(line2);
  }
}

void Ui::toast(const char* msg, uint32_t holdMs) {
  if (!msg) return;
  int16_t w = (int16_t)strlen(msg) * CHAR_W + 16;
  int16_t h = 20;
  int16_t x = (SCREEN_W - w) / 2;
  int16_t y = SCREEN_H - h - 8;
  M5.Lcd.fillRoundRect(x, y, w, h, 4, COLOR_ACCENT);
  M5.Lcd.setTextColor(COLOR_BG, COLOR_ACCENT);
  M5.Lcd.setCursor(x + 8, y + 6);
  M5.Lcd.print(msg);
  delay(holdMs);
}

}  // namespace journal
