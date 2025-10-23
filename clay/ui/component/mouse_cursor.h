// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_MOUSE_CURSOR_H_
#define CLAY_UI_COMPONENT_MOUSE_CURSOR_H_

#include <string>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/platform/cursor_types.h"

namespace clay {

// It allows specifying the coordinates of the cursor's hotspot, which will be
// clamped to the boundaries of the cursor image. If none are specified, the
// coordinates of the hotspot are read from the file itself (for CUR and XBM
// files) or are set to the top left corner of the image.
struct Hotspot {
  int x;
  int y;
  explicit Hotspot(int x_val = -1, int y_val = -1) : x(x_val), y(y_val) {}
};

struct Cursor {
  CursorTypes type;
  std::string val;
  Hotspot hspot;
  Cursor(CursorTypes m_type, const std::string& m_str, int x = -1, int y = -1)
      : type(m_type), val(m_str), hspot(x, y) {}
};

class MouseCursor {
 public:
  MouseCursor() = default;
  explicit MouseCursor(const std::vector<std::string>& cursor_vec) {
    // see [https://developer.mozilla.org/zh-CN/docs/Web/CSS/cursor]
    // cursor_vec look like:
    // image/path
    // image/path x y
    // keyword
    const char identify = ' ';

    for (auto& one_url : cursor_vec) {
      int index = one_url.find(identify);
      if (index == -1) {
        cursors_.emplace_back(CursorTypeUtil::ParseCursorType(one_url),
                              one_url);
        continue;
      }

      const int len = one_url.length();
      int x = -1;
      int y = -1;
      std::string cursor_path = one_url.substr(0, index);
      std::string temp = "";

      // parse x
      while (index < len && one_url[index] == ' ') {
        ++index;
      }

      while (index < len) {
        if (one_url[index] == ' ') break;
        temp += one_url[index];
        index++;
      }

      if (!temp.empty()) {
        x = std::atoi(temp.c_str());
        temp = "";
      }

      // parse y
      while (index < len && one_url[index] == ' ') {
        ++index;
      }

      while (index < len) {
        if (one_url[index] == ' ') break;
        temp += one_url[index];
        index++;
      }

      if (!temp.empty()) {
        y = std::atoi(temp.c_str());
      }
      cursors_.emplace_back(CursorTypeUtil::ParseCursorType(cursor_path),
                            cursor_path, x, y);
    }
  }

  const std::vector<Cursor>& GetCursors() { return cursors_; }
  void AddCursor(const Cursor& cursor) { cursors_.emplace_back(cursor); }

 private:
  std::vector<Cursor> cursors_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_MOUSE_CURSOR_H_
