// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_TEXT_INPUT_HISTORY_ACTION_H_
#define CLAY_UI_COMMON_TEXT_INPUT_HISTORY_ACTION_H_

#include <cstdint>

namespace clay {

enum class TextInputHistoryAction : uint8_t {
  kUndo,
  kRedo,
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_TEXT_INPUT_HISTORY_ACTION_H_
