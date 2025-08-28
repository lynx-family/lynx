// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_bounce.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIBounce::UIBounce(LynxContext* context, int sign, const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  overflow_ = {false, false};
}

void UIBounce::OnPropUpdate(const std::string& name,
                            const lepus::Value& value) {
  if (name == "direction") {
    const auto& val = value.StdString();
    if (val == "left" || val == "top") {
      is_lower_ = true;
    } else if (val == "right" || val == "bottom") {
      is_lower_ = false;
    }
  }
  UIView::OnPropUpdate(name, value);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
