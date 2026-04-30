#include "Viewer.h"

#include <M5Stack.h>

#include "Config.h"
#include "Storage.h"
#include "Ui.h"

namespace journal {

bool Viewer::open(const char* path) {
  path_ = path;
  body_ = "";
  scroll_ = 0;
  if (!Storage::readEntry(path, body_, MAX_ENTRY_BYTES)) return false;
  layout();
  return true;
}

void Viewer::layout() {
  lines_.clear();
  const size_t maxCols = (size_t)Ui::cols();
  size_t i = 0, lineStart = 0, lastSpace = (size_t)-1;
  while (i < body_.length()) {
    char c = body_[i];
    if (c == '\n') {
      lines_.push_back({lineStart, i});
      ++i;
      lineStart = i;
      lastSpace = (size_t)-1;
      continue;
    }
    if (c == ' ') lastSpace = i;
    ++i;
    if (i - lineStart >= maxCols) {
      bool haveBreak = (lastSpace != (size_t)-1 && lastSpace >= lineStart);
      size_t lineEnd   = haveBreak ? lastSpace      : i;
      size_t nextStart = haveBreak ? lastSpace + 1 : i;
      lines_.push_back({lineStart, lineEnd});
      lineStart = nextStart;
      lastSpace = (size_t)-1;
    }
  }
  lines_.push_back({lineStart, body_.length()});
}

void Viewer::scrollUp() {
  if (scroll_ > 0) --scroll_;
}

void Viewer::scrollDown() {
  size_t visible = (size_t)Ui::rows();
  if (scroll_ + visible < lines_.size()) ++scroll_;
}

void Viewer::render() {
  M5.Lcd.fillRect(0, Ui::bodyTop(), SCREEN_W, Ui::bodyHeight(), COLOR_BG);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(COLOR_FG, COLOR_BG);
  size_t visible = (size_t)Ui::rows();
  for (size_t row = 0; row < visible; ++row) {
    size_t li = scroll_ + row;
    if (li >= lines_.size()) break;
    int16_t y = Ui::bodyTop() + (int16_t)row * LINE_H;
    M5.Lcd.setCursor(0, y);
    const Line& L = lines_[li];
    for (size_t i = L.start; i < L.end; ++i) M5.Lcd.print(body_[i]);
  }
}

}  // namespace journal
