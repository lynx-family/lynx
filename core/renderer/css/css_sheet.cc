// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_sheet.h"

#include "core/renderer/css/css_selector_constants.h"

namespace lynx {
namespace tasm {

CSSSheet::CSSSheet(const std::string& str)
    : type_(0), selector_(str), parent_(nullptr) {
  ConfirmType();
}

void CSSSheet::ConfirmType() {
  if (selector_.empty()) return;

  const auto& name = selector_.str();
  constexpr const static char* kColon = ":";

  // TODO(yuyang), optimize this.
  // If it is a pseudo-class or pseudo-element, it must contain ':'
  if (name.find(kColon) != std::string::npos) {
    if (name.find(kCSSSelectorPlaceholder) != std::string::npos) {
      type_ |= PLACEHOLDER_SELECT;
    } else if (name.find(kCSSSelectorFirstChild) != std::string::npos) {
      type_ |= FIRST_CHILD_SELECT;
    } else if (name.find(kCSSSelectorLastChild) != std::string::npos) {
      type_ |= LAST_CHILD_SELECT;
    } else if (name.find(kCSSSelectorNot) != std::string::npos) {
      type_ |= NOT_SELECT;
    } else if (name.find(kCSSSelectorSelection) != std::string::npos) {
      type_ |= SELECTION_SELECT;
    } else if (name.find(kCSSSelectorPseudoActive) != std::string::npos) {
      type_ |= PSEUDO_ACTIVE_SELECT;
    } else if (name.find(kCSSSelectorPseudoFocus) != std::string::npos) {
      type_ |= PSEUDO_FOCUS_SELECT;
    } else if (name.find(kCSSSelectorPseudoHover) != std::string::npos) {
      type_ |= PSEUDO_HOVER_SELECT;
    }
  }

  const auto& type = name.substr(0, 1);
  if (type == kCSSSelectorClass) {
    type_ |= CLASS_SELECT;
    name_ = name.substr(1);
  } else if (type == kCSSSelectorID) {
    type_ |= ID_SELECT;
    name_ = name.substr(1);
  } else if (type == kCSSSelectorAll) {
    type_ |= ALL_SELECT;
    name_ = BASE_STATIC_STRING(kCSSSelectorAll);
  } else {
    type_ |= NAME_SELECT;
    name_ = name;
  }
}

bool CSSSheet::IsTouchPseudo() const {
  return type_ &
         (PSEUDO_ACTIVE_SELECT | PSEUDO_FOCUS_SELECT | PSEUDO_HOVER_SELECT);
}

}  // namespace tasm
}  // namespace lynx
