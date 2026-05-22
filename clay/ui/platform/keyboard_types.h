// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_KEYBOARD_TYPES_H_
#define CLAY_UI_PLATFORM_KEYBOARD_TYPES_H_

namespace clay {

enum class KeyboardInputType {
  kClassText = 0,
  kClassNumber,
  kClassPhone,
  kUrl,
  kEmailAddress,
  kPassword,
};

enum class KeyboardAction {
  kMultiLine = 0,
  kGo,
  kSearch,
  kSend,
  kNext,
  kDone,
  kPrevious,
};

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_KEYBOARD_TYPES_H_
