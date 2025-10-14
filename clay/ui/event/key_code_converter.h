// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_EVENT_KEY_CODE_CONVERTER_H_
#define CLAY_UI_EVENT_KEY_CODE_CONVERTER_H_

#include <string>

#include "clay/ui/event/keyboard_key.h"

namespace clay {
class KeyCodeConverter {
 public:
  // used for reporting keycode to lynx
  // align with web standard KeyboardEvent.key
  static std::string ConvertToWebKey(LogicalKeyboardKey key_code,
                                     const std::string& character);
};
}  // namespace clay

#endif  // CLAY_UI_EVENT_KEY_CODE_CONVERTER_H_
