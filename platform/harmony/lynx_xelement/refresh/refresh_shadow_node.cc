// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/refresh/refresh_shadow_node.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {
RefreshShadowNode::RefreshShadowNode(int sign, const std::string& tag)
    : ShadowNode(sign, tag) {
  SetCustomMeasureFunc(this);
}

LayoutResult RefreshShadowNode::Measure(float width, MeasureMode width_mode,
                                        float height, MeasureMode height_mode,
                                        bool final_measure) {
  for (ShadowNode* child : GetChildren()) {
    if (strcmp(child->Tag(), "refresh-header") == 0) {
      child->MeasureLayoutNode(width, width_mode, height,
                               MeasureMode::Indefinite, final_measure);
    } else {
      child->MeasureLayoutNode(width, width_mode, height, MeasureMode::Definite,
                               final_measure);
    }
  }
  return LayoutResult(width, height, 0.f);
}

void RefreshShadowNode::Align() {
  for (ShadowNode* child : GetChildren()) {
    child->AlignLayoutNode(0.f, 0.f);
  }
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
