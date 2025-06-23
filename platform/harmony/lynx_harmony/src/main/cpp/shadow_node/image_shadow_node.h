// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_IMAGE_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_IMAGE_SHADOW_NODE_H_

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {
class ImageShadowNode : public ShadowNode, public CustomMeasureFunc {
 public:
  ImageShadowNode(int sign, const std::string& tag);
  static ShadowNode* Make(int sign, const std::string& tag) {
    return new ImageShadowNode(sign, tag);
  }
  LayoutResult Measure(float width, MeasureMode width_mode, float height,
                       MeasureMode height_mode, bool final_measure) override;
  void JustSize(bool auto_size, float bitmap_width, float bitmap_height,
                float width, float height);
  void Align() override;

 private:
  bool auto_size_{false};
  float bitmap_width_{0.f};
  float bitmap_height_{0.f};
  bool exactly_{true};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_IMAGE_SHADOW_NODE_H_
