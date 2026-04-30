#pragma once

#include <Arduino.h>

#include <vector>

namespace journal {

class EntryList {
 public:
  bool refresh();
  void render();
  void moveUp();
  void moveDown();
  bool empty() const { return names_.empty(); }
  String selectedName() const;
  String selectedPath() const;

 private:
  std::vector<String> names_;
  size_t              sel_    = 0;
  size_t              scroll_ = 0;
  void ensureSelVisible();
};

}  // namespace journal
