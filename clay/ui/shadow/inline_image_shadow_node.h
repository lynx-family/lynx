// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_INLINE_IMAGE_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_INLINE_IMAGE_SHADOW_NODE_H_

#include <string>

#include "clay/ui/component/layout_controller.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

class InlineImageShadowNode : public ShadowNode {
 public:
  InlineImageShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~InlineImageShadowNode() = default;

  void PreLayout(PreLayoutContext* context) override;
  void TextLayout(LayoutContext* context) override;

  int placeholder_index() const { return placeholder_index_; }

  size_t StartGlyph() { return start_glyph_; }
  size_t EndGlyph() { return end_glyph_; }

  bool IsInlineImageShadowNode() override { return true; }

  bool IsVirtual() override { return true; }

  void SetBaselineOffset(double baseline_offset) override {
    baseline_offset_ = baseline_offset;
  }

 private:
  int placeholder_index_ = -1;
  double baseline_offset_ = 0.f;
  size_t start_glyph_ = 0;
  size_t end_glyph_ = 0;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_INLINE_IMAGE_SHADOW_NODE_H_
