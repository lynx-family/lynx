// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_IME_UTILS_H_
#define CLAY_UI_COMPONENT_EDITABLE_IME_UTILS_H_

#include <string>

#include "clay/ui/component/component_constants.h"
#include "clay/ui/platform/keyboard_types.h"

namespace clay {

KeyboardInputType ConvertInputType(const std::string& type);

KeyboardAction ConvertConfirmType(
    const std::string& type,
    KeyboardAction default_value = KeyboardAction::kMultiLine);

const char* ToKeyboardActionType(KeyboardAction action);

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_IME_UTILS_H_
