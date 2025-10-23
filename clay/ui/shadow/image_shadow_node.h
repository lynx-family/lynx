// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_IMAGE_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_IMAGE_SHADOW_NODE_H_

#include <string>

#include "clay/ui/component/measurable.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

class ImageShadowNode : public ShadowNode, public CustomMeasurable {
 public:
  ImageShadowNode(ShadowNodeOwner* owner, std::string tag, int id);

  bool IsImageShadowNode() override { return true; }

  void AdjustSizeIfNeeded(bool auto_size, float bitmap_width,
                          float bitmap_height);

  // |CustomMeasurable|
  MeasureResult Measure(const MeasureConstraint& constraint) override;
  void Align() override {}
  CustomMeasurable* GetCustomMeasurable() override { return this; }

 private:
  bool auto_size_ = false;
  float bitmap_width_ = 0.f;
  float bitmap_height_ = 0.f;
  std::mutex mutex_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_IMAGE_SHADOW_NODE_H_
