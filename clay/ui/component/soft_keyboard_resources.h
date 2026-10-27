// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SOFT_KEYBOARD_RESOURCES_H_
#define CLAY_UI_COMPONENT_SOFT_KEYBOARD_RESOURCES_H_

#include <cstdint>

namespace clay {
namespace keyboard_res {

extern const uint8_t kHideKeyboard[];
extern const uint8_t kArrowLeft[];
extern const uint8_t kArrowRight[];
extern const uint8_t kBackspace[];
extern const int kHideKeyboardLength;
extern const int kArrowLeftLength;
extern const int kArrowRightLength;
extern const int kBackspaceLength;
extern const char* kHideKeyboardUrl;
extern const char* kArrowLeftUrl;
extern const char* kArrowRightUrl;
extern const char* kBackspaceUrl;
extern const char* kKeyBackground;

}  // namespace keyboard_res
}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SOFT_KEYBOARD_RESOURCES_H_
