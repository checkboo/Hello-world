#pragma once

#include <Arduino.h>

#include <vector>

#include "BTKeyboard.h"

namespace journal {

class Editor {
 public:
  void open(const char* path, const String& initialBody);
  void onKey(const KeyEvent& k);
  void tick();           // call from loop(); handles autosave
  void render();         // full repaint of body region
  bool dirty() const { return dirty_; }
  bool saveNow();        // force flush

  const char* path() const { return path_.c_str(); }

 private:
  std::vector<char> buf_;
  size_t            cursor_ = 0;
  String            path_;
  bool              dirty_  = false;
  uint32_t          lastKeyMs_ = 0;
  uint32_t          lastBlinkMs_ = 0;
  bool              blinkOn_ = true;

  // Wrapped layout: each entry is a half-open byte range [start, end) into buf_.
  struct Line { size_t start; size_t end; };
  std::vector<Line> lines_;
  size_t            scrollLine_ = 0;
  bool              layoutDirty_ = true;

  void insertChar(char c);
  void backspace();
  void delForward();
  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();
  void moveHome();
  void moveEnd();
  void layout();
  size_t cursorLine() const;
  void   ensureCursorVisible();
};

}  // namespace journal
