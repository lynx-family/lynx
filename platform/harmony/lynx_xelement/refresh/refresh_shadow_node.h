// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_REFRESH_REFRESH_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_REFRESH_REFRESH_SHADOW_NODE_H_

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

class RefreshShadowNode : public ShadowNode, public CustomMeasureFunc {
 public:
  ~RefreshShadowNode() override = default;
  RefreshShadowNode(int sign, const std::string& tag);
  static ShadowNode* Make(int sign, const std::string& tag) {
    return new RefreshShadowNode(sign, tag);
  }
  LayoutResult Measure(float width, MeasureMode width_mode, float height,
                       MeasureMode height_mode, bool final_measure) override;
  void Align() override;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_REFRESH_REFRESH_SHADOW_NODE_H_
