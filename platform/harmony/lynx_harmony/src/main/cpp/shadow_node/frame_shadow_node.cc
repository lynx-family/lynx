// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/frame_shadow_node.h"

#include "base/include/float_comparison.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace tasm {
namespace harmony {

FrameShadowNode::FrameShadowNode(int sign, const std::string& tag)
    : ShadowNode(sign, tag) {
  SetCustomMeasureFunc(this);
}

LayoutResult FrameShadowNode::Measure(float width, MeasureMode width_mode,
                                      float height, MeasureMode height_mode,
                                      bool final_measure) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, FRAME_SHADOW_NODE_MEASURE, "sign",
                    Signature(), "input_width", width, "input_height", height,
                    "input_width_mode", width_mode, "input_height_mode",
                    height_mode);

  LayoutResult result(width, height, 0.f);
  if (!base::IsZero(intrinsic_width_) || !base::IsZero(intrinsic_height_)) {
    result.width_ =
        width_mode == MeasureMode::Definite ? width : intrinsic_width_;
    result.height_ =
        height_mode == MeasureMode::Definite ? height : intrinsic_height_;
  }

  TRACE_EVENT_END(LYNX_TRACE_CATEGORY, [&result](perfetto::EventContext ctx) {
    ctx.event()->add_debug_annotations("width", std::to_string(result.width_));
    ctx.event()->add_debug_annotations("height",
                                       std::to_string(result.height_));
  });
  return result;
}

void FrameShadowNode::UpdateIntrinsicSize(float intrinsic_width,
                                          float intrinsic_height) {
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY,
                      FRAME_SHADOW_NODE_UPDATE_INTRINSIC_SIZE, "sign",
                      Signature(), "old_width", intrinsic_width_, "old_height",
                      intrinsic_height_, "new_width", intrinsic_width,
                      "new_height", intrinsic_height);

  if (base::FloatsEqual(intrinsic_width_, intrinsic_width) &&
      base::FloatsEqual(intrinsic_height_, intrinsic_height)) {
    return;
  }
  intrinsic_width_ = intrinsic_width;
  intrinsic_height_ = intrinsic_height;
  MarkDirty();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
