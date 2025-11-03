// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/overlay/overlay_shadow_node.h"

#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
namespace lynx {
namespace tasm {
namespace harmony {
OverlayShadowNode::OverlayShadowNode(int sign, const std::string& tag)
    : ShadowNode(sign, tag) {
  SetCustomMeasureFunc(this);
}

LayoutResult OverlayShadowNode::Measure(float width, MeasureMode width_mode,
                                        float height, MeasureMode height_mode,
                                        bool final_measure) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, OVERLAY_SHADOW_NODE_MEASURE);
  if (!GetChildren().empty()) {
    const ShadowNode* child = GetChildren()[0];

    const float screen_width =
        NodeManager::GetScreenWidth() / context_->ScaledDensity();
    const float screen_height =
        NodeManager::GetScreenHeight() / context_->ScaledDensity();

    child->MeasureLayoutNode(screen_width, MeasureMode::Definite, screen_height,
                             MeasureMode::Definite, false);
  }
  return LayoutResult(0.f, 0.f, 0.f);
}

void OverlayShadowNode::Align() {
  if (!GetChildren().empty()) {
    const ShadowNode* child = GetChildren()[0];
    child->AlignLayoutNode(0.f, 0.f);
  }
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
