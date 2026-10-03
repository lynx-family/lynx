// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_INLINE_TRUNCATION_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_INLINE_TRUNCATION_SHADOW_NODE_H_

#include <string>
#include <utility>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/shadow/base_text_shadow_node.h"

namespace clay {

class InlineTruncationShadowNode : public BaseTextShadowNode {
 public:
  InlineTruncationShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~InlineTruncationShadowNode() override = default;

  bool IsInlineTruncationShadowNode() override { return true; }

  void TextLayout(LayoutContext* context) override;

  FloatSize CalculateTruncatedSize();
  void UpdateTruncatedSize(float width, float height);

  void SetNeedLayout(bool need_layout) { need_layout_ = need_layout; }
  void SetNeedMount(bool need_mount) { need_mount_ = need_mount; }
  bool IfNeedMount() const { return need_mount_; }

  void SetEllipsis(std::u16string ellipsis) { ellipsis_ = std::move(ellipsis); }
  size_t StartGlyph() const { return start_glyph_; }
  size_t EndGlyph() const { return end_glyph_; }

  bool IsVirtual() override { return true; }

 private:
  void LayoutTruncation(LayoutContext* context);

  std::u16string ellipsis_;
  size_t start_glyph_ = 0;
  size_t end_glyph_ = 0;
  bool need_layout_ = false;
  bool need_mount_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_INLINE_TRUNCATION_SHADOW_NODE_H_
