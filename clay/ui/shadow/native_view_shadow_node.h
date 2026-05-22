// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_NATIVE_VIEW_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_NATIVE_VIEW_SHADOW_NODE_H_

#include "clay/ui/component/measurable.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

// This is only used for internal NativeViews. For external NativeViews
// (i.e. Lynx NativeViews), the ShadowNode is created and managed by Lynx.
class NativeViewShadowNode : public ShadowNode, public CustomMeasurable {
 public:
  using ShadowNode::ShadowNode;

  CustomMeasurable* GetCustomMeasurable() override { return this; }

  // CustomMeasurable
  MeasureResult Measure(const MeasureConstraint& constraint) override;
  void Align() override {
    // For now we do not support custom alignment for internal NativeViews.
  }
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_NATIVE_VIEW_SHADOW_NODE_H_
