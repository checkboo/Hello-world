#include "Editor.h"

#include <M5Stack.h>

#include "Config.h"
#include "Storage.h"
#include "Ui.h"

namespace journal {

void Editor::open(const char* path, const String& initial) {
  path_ = path;
  buf_.assign(initial.c_str(), initial.c_str() + initial.length());
  cursor_       = buf_.size();
  dirty_        = false;
  lastKeyMs_    = 0;
  scrollLine_   = 0;
  layoutDirty_  = true;
  layout();
  ensureCursorVisible();
}

void Editor::insertChar(char c) {
  if (buf_.size() >= MAX_ENTRY_BYTES) return;
  buf_.insert(buf_.begin() + cursor_, c);
  ++cursor_;
  dirty_ = true;
  layoutDirty_ = true;
}

void Editor::backspace() {
  if (cursor_ == 0) return;
  buf_.erase(buf_.begin() + (cursor_ - 1));
  --cursor_;
  dirty_ = true;
  layoutDirty_ = true;
}

void Editor::delForward() {
  if (cursor_ >= buf_.size()) return;
  buf_.erase(buf_.begin() + cursor_);
  dirty_ = true;
  layoutDirty_ = true;
}

void Editor::moveLeft()  { if (cursor_) --cursor_; }
void Editor::moveRight() { if (cursor_ < buf_.size()) ++cursor_; }

size_t Editor::cursorLine() const {
  for (size_t i = 0; i < lines_.size(); ++i) {
    if (cursor_ >= lines_[i].start && cursor_ <= lines_[i].end) return i;
  }
  return lines_.empty() ? 0 : lines_.size() - 1;
}

void Editor::moveUp() {
  if (lines_.empty()) return;
  size_t cl = cursorLine();
  if (cl == 0) { cursor_ = 0; return; }
  size_t col = cursor_ - lines_[cl].start;
  size_t target = lines_[cl - 1].start + col;
  if (target > lines_[cl - 1].end) target = lines_[cl - 1].end;
  cursor_ = target;
}

void Editor::moveDown() {
  if (lines_.empty()) return;
  size_t cl = cursorLine();
  if (cl + 1 >= lines_.size()) { cursor_ = buf_.size(); return; }
  size_t col = cursor_ - lines_[cl].start;
  size_t target = lines_[cl + 1].start + col;
  if (target > lines_[cl + 1].end) target = lines_[cl + 1].end;
  cursor_ = target;
}

void Editor::moveHome() {
  size_t cl = cursorLine();
  if (cl < lines_.size()) cursor_ = lines_[cl].start;
}

void Editor::moveEnd() {
  size_t cl = cursorLine();
  if (cl < lines_.size()) cursor_ = lines_[cl].end;
}

void Editor::layout() {
  if (!layoutDirty_) return;
  lines_.clear();
  const size_t maxCols = (size_t)Ui::cols();
  size_t i = 0, lineStart = 0, lastSpace = (size_t)-1;
  while (i < buf_.size()) {
    char c = buf_[i];
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
      size_t lineEnd  = haveBreak ? lastSpace      : i;
      size_t nextStart = haveBreak ? lastSpace + 1 : i;
      lines_.push_back({lineStart, lineEnd});
      lineStart = nextStart;
      lastSpace = (size_t)-1;
    }
  }
  lines_.push_back({lineStart, buf_.size()});
  layoutDirty_ = false;
}

void Editor::ensureCursorVisible() {
  layout();
  size_t cl = cursorLine();
  size_t visible = (size_t)Ui::rows();
  if (cl < scrollLine_) scrollLine_ = cl;
  else if (cl >= scrollLine_ + visible) scrollLine_ = cl - visible + 1;
}

void Editor::onKey(const InputKey& k) {
  if (!k.down) return;
  lastKeyMs_ = millis();

  if (k.ctrl) {
    if (k.code == 's' || k.code == 'S') { saveNow(); return; }
    if (k.code == 'a' || k.code == 'A') { moveHome(); ensureCursorVisible(); return; }
    if (k.code == 'e' || k.code == 'E') { moveEnd();  ensureCursorVisible(); return; }
    return;
  }

  switch (k.code) {
    case KEY_NONE:                                    return;
    case KEY_BACKSPACE: backspace();                  break;
    case KEY_DELETE:    delForward();                 break;
    case KEY_ENTER:     insertChar('\n');             break;
    case KEY_TAB:       insertChar(' '); insertChar(' '); break;
    case KEY_LEFT:      moveLeft();                   break;
    case KEY_RIGHT:     moveRight();                  break;
    case KEY_UP:        layout(); moveUp();           break;
    case KEY_DOWN:      layout(); moveDown();         break;
    case KEY_HOME:      layout(); moveHome();         break;
    case KEY_END:       layout(); moveEnd();          break;
    case KEY_ESC:                                     return;  // handled by AppState
    case KEY_PAGEUP:
    case KEY_PAGEDOWN:                                return;  // unimplemented
    default:
      if (k.code >= 0x20 && k.code <= 0x7E) insertChar((char)k.code);
      break;
  }
  ensureCursorVisible();
}

bool Editor::saveNow() {
  if (!dirty_) return true;
  bool ok = Storage::saveAtomic(path_.c_str(), buf_.data(), buf_.size());
  if (ok) dirty_ = false;
  return ok;
}

void Editor::tick() {
  uint32_t now = millis();
  if (dirty_ && now - lastKeyMs_ > AUTOSAVE_MS) {
    saveNow();
  }
  if (now - lastBlinkMs_ > 500) {
    lastBlinkMs_ = now;
    blinkOn_     = !blinkOn_;
    // Cheap: redraw cursor row only would be ideal, but for simplicity
    // we let render() handle full repaint on demand.
  }
}

void Editor::render() {
  layout();
  M5.Lcd.fillRect(0, Ui::bodyTop(), SCREEN_W, Ui::bodyHeight(), COLOR_BG);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(COLOR_FG, COLOR_BG);

  size_t visible = (size_t)Ui::rows();
  size_t cl      = cursorLine();
  size_t curCol  = cursor_ - lines_[cl].start;

  for (size_t row = 0; row < visible; ++row) {
    size_t lineIdx = scrollLine_ + row;
    if (lineIdx >= lines_.size()) break;
    int16_t y = Ui::bodyTop() + (int16_t)row * LINE_H;
    M5.Lcd.setCursor(0, y);
    const Line& L = lines_[lineIdx];
    for (size_t i = L.start; i < L.end; ++i) {
      char c = buf_[i];
      M5.Lcd.print(c == '\n' ? ' ' : c);
    }
    if (lineIdx == cl) {
      int16_t cx = (int16_t)curCol * CHAR_W;
      M5.Lcd.drawFastVLine(cx, y, LINE_H - 1, COLOR_ACCENT);
    }
  }
}

}  // namespace journal
