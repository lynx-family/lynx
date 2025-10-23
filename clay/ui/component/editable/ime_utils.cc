// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/ime_utils.h"

namespace clay {
KeyboardInputType ConvertInputType(const std::string& type) {
  if (type == attr_value::kInputTypeNumber ||
      type == attr_value::kInputTypeDigit) {
    return KeyboardInputType::kClassNumber;
  }
  if (type == attr_value::kInputTypeTel) {
    return KeyboardInputType::kClassPhone;
  }
  if (type == attr_value::kInputTypePassword) {
    return KeyboardInputType::kPassword;
  }
  if (type == attr_value::kInputTypeEmail) {
    return KeyboardInputType::kEmailAddress;
  }
  return KeyboardInputType::kClassText;
}

KeyboardAction ConvertConfirmType(const std::string& type,
                                  KeyboardAction default_value) {
  if (type == attr_value::kInputActionSend) {
    return KeyboardAction::kSend;
  }
  if (type == attr_value::kInputActionSearch) {
    return KeyboardAction::kSearch;
  }
  if (type == attr_value::kInputActionGo) {
    return KeyboardAction::kGo;
  }
  if (type == attr_value::kInputActionDone) {
    return KeyboardAction::kDone;
  }
  if (type == attr_value::kInputActionNext) {
    return KeyboardAction::kNext;
  }
  return default_value;
}

const char* ToKeyboardActionType(KeyboardAction action) {
  switch (action) {
    case KeyboardAction::kMultiLine:
      return "TextInputAction.newline";
    case KeyboardAction::kDone:
      return "TextInputAction.done";
    case KeyboardAction::kSearch:
      return "TextInputAction.search";
    case KeyboardAction::kGo:
      return "TextInputAction.go";
    case KeyboardAction::kNext:
      return "TextInputAction.next";
    default:
      return "TextInputAction.none";
  }
}
}  // namespace clay
