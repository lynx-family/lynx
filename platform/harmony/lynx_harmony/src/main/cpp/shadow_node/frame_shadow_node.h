// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_FRAME_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_FRAME_SHADOW_NODE_H_

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

class FrameShadowNode : public ShadowNode, public CustomMeasureFunc {
 public:
  FrameShadowNode(int sign, const std::string& tag);
  static ShadowNode* Make(int sign, const std::string& tag) {
    return new FrameShadowNode(sign, tag);
  }

  LayoutResult Measure(float width, MeasureMode width_mode, float height,
                       MeasureMode height_mode, bool final_measure) override;
  void Align() override {}
  void UpdateIntrinsicSize(float intrinsic_width, float intrinsic_height);

 private:
  float intrinsic_width_{0.f};
  float intrinsic_height_{0.f};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_FRAME_SHADOW_NODE_H_
