// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_INLINE_VIEW_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_INLINE_VIEW_SHADOW_NODE_H_

#include <string>

#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

class InlineViewShadowNode : public ShadowNode {
 public:
  InlineViewShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~InlineViewShadowNode() override = default;

  void PreLayout(PreLayoutContext* context) override;

  bool IsInlineViewShadowNode() override { return true; }

  bool IsVirtual() override { return true; }

  void TextLayout(LayoutContext* context) override;

  size_t StartGlyph() { return start_glyph_; }
  size_t EndGlyph() { return end_glyph_; }

  int placeholder_index() const { return placeholder_index_; }

  void ResetTextLayout();

  void SetBaselineOffset(double baseline_offset) override {
    baseline_offset_ = baseline_offset;
  }

  MeasureResult MeasureNativeNode(const MeasureConstraint& constraint);
  void AlignNativeNode(float top, float left);

 private:
  int placeholder_index_ = -1;
  size_t start_glyph_ = 0;
  size_t end_glyph_ = 0;
  double baseline_offset_ = 0.f;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_INLINE_VIEW_SHADOW_NODE_H_
