// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_FRAME_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_FRAME_SHADOW_NODE_H_

#include <mutex>
#include <optional>
#include <string>

#include "clay/ui/component/measurable.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

class FrameShadowNode : public ShadowNode, public CustomMeasurable {
 public:
  FrameShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~FrameShadowNode() override = default;

  bool IsFrameShadowNode() override { return true; }
  CustomMeasurable* GetCustomMeasurable() override { return this; }

  MeasureResult Measure(const MeasureConstraint& constraint) override;
  void Align() override {}

  void SetIntrinsicContentSize(std::optional<float> width,
                               std::optional<float> height);

 private:
  static float ResolveDimension(std::optional<float> intrinsic,
                                std::optional<float> constraint,
                                MeasureMode mode);

  std::optional<float> intrinsic_width_;
  std::optional<float> intrinsic_height_;
  std::mutex mutex_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_FRAME_SHADOW_NODE_H_
