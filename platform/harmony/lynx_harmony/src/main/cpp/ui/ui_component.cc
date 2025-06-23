// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_component.h"

#include <string>

#include "core/renderer/ui_component/list/list_types.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIComponent::UIComponent(LynxContext* context, int sign, const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {}

void UIComponent::OnPropUpdate(const std::string& name,
                               const lepus::Value& value) {
  if (name == list::kItemKey && value.IsString()) {
    item_key_ = value.StdString();
  } else if (name == "z-index" && value.IsNumber()) {
    z_index_ = static_cast<int>(value.Number());
  } else {
    UIView::OnPropUpdate(name, value);
  }
}

void UIComponent::OnNodeReady() {
  UIView::OnNodeReady();
  if (parent_ && parent_->IsList()) {
    NodeManager::Instance().SetAttributeWithNumberValue(DrawNode(),
                                                        NODE_Z_INDEX, z_index_);
  }
  if (node_ready_listener_) {
    node_ready_listener_->OnComponentNodeReady(this);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
