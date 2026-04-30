#include "EntryList.h"

#include <M5Stack.h>

#include "Config.h"
#include "Storage.h"
#include "Ui.h"

namespace journal {

bool EntryList::refresh() {
  bool ok = Storage::listEntries(names_);
  if (sel_ >= names_.size()) sel_ = 0;
  ensureSelVisible();
  return ok;
}

void EntryList::moveUp()   { if (sel_ > 0) { --sel_; ensureSelVisible(); } }
void EntryList::moveDown() { if (sel_ + 1 < names_.size()) { ++sel_; ensureSelVisible(); } }

void EntryList::ensureSelVisible() {
  size_t visible = (size_t)Ui::rows();
  if (sel_ < scroll_) scroll_ = sel_;
  else if (sel_ >= scroll_ + visible) scroll_ = sel_ - visible + 1;
}

String EntryList::selectedName() const {
  if (names_.empty()) return "";
  return names_[sel_];
}

String EntryList::selectedPath() const {
  if (names_.empty()) return "";
  return String(JOURNAL_DIR) + "/" + names_[sel_];
}

void EntryList::render() {
  M5.Lcd.fillRect(0, Ui::bodyTop(), SCREEN_W, Ui::bodyHeight(), COLOR_BG);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextSize(1);

  if (names_.empty()) {
    M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
    M5.Lcd.setCursor(8, Ui::bodyTop() + 8);
    M5.Lcd.print("No entries on SD.");
    return;
  }

  size_t visible = (size_t)Ui::rows();
  for (size_t row = 0; row < visible; ++row) {
    size_t i = scroll_ + row;
    if (i >= names_.size()) break;
    int16_t y = Ui::bodyTop() + (int16_t)row * LINE_H;
    bool isSel = (i == sel_);
    if (isSel) M5.Lcd.fillRect(0, y, SCREEN_W, LINE_H, COLOR_ACCENT);
    M5.Lcd.setTextColor(isSel ? COLOR_BG : COLOR_FG, isSel ? COLOR_ACCENT : COLOR_BG);
    M5.Lcd.setCursor(8, y);
    M5.Lcd.print(names_[i]);
  }
}

}  // namespace journal
