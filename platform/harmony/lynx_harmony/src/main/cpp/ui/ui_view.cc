// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIBase* UIView::Make(LynxContext* context, int sign, const std::string& tag) {
  return new UIView(context, ARKUI_NODE_CUSTOM, sign, tag);
}

void UIView::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  // TODO(renzhongyue): prop setters here
  UIBase::OnPropUpdate(name, value);
}

UIView::UIView(LynxContext* context, ArkUI_NodeType type, int sign,
               const std::string& tag)
    : UIBase(context, type, sign, tag) {
  // default overflow.
  overflow_ = {true, true};
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
