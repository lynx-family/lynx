// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_CURSOR_HANDLER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_CURSOR_HANDLER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/shell/platform/windows/window_binding_handler.h"

namespace clay {

class FlutterWindowsEngine;

// Handler for the cursor system.
class CursorHandler {
 public:
  explicit CursorHandler(clay::FlutterWindowsEngine* engine);

  void ActivateSystemCursor(int type, const char* path);

 private:
  // The Flutter engine that will be notified for cursor updates.
  FlutterWindowsEngine* engine_;

  // The cache map for custom cursors.
  std::unordered_map<std::string, HCURSOR> custom_cursors_;

  fml::WeakPtrFactory<CursorHandler> weak_factory_;
};

// Create a cursor from a rawBGRA buffer and the cursor info.
HCURSOR GetCursorFromBuffer(const std::vector<uint8_t>& buffer, double hot_x,
                            double hot_y, int width, int height);

// Get the corresponding mask bitmap from the source bitmap.
void GetMaskBitmaps(HBITMAP bitmap, HBITMAP& mask_bitmap);

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_CURSOR_HANDLER_H_
