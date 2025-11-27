// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_TEXT_INPUT_PLUGIN_DELEGATE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_TEXT_INPUT_PLUGIN_DELEGATE_H_

#include "clay/gfx/geometry/float_rect.h"

namespace clay {

class TextInputPluginDelegate {
 public:
  // Notifies the delegate of the updated the cursor rect in Flutter root view
  // coordinates.
  virtual void OnCursorRectUpdated(const FloatRect& rect) = 0;

  // Notifies the delegate that the system IME composing state should be reset.
  virtual void OnResetImeComposing() = 0;

  virtual void OnTextInputClientChange(int client_id) = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_TEXT_INPUT_PLUGIN_DELEGATE_H_
