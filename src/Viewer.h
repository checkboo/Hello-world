#pragma once

#include <Arduino.h>

#include <vector>

namespace journal {

class Viewer {
 public:
  bool open(const char* path);
  void render();
  void scrollUp();
  void scrollDown();
  const char* path() const { return path_.c_str(); }

 private:
  String            body_;
  String            path_;
  struct Line { size_t start; size_t end; };
  std::vector<Line> lines_;
  size_t            scroll_ = 0;
  void layout();
};

}  // namespace journal
