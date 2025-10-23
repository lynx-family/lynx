// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_SHADOW_LAYOUT_CONTEXT_H_
#define CLAY_UI_SHADOW_SHADOW_LAYOUT_CONTEXT_H_

#include <vector>

#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/shadow/inline_view_shadow_node.h"

namespace clay {

class ShadowLayoutContextMeasure : public LayoutContext {
 public:
  // Width for text paragraph layout.
  float layout_width_ = 0.f;

  // Indicate the intrinsic width after layout using |layout_width_|.
  int measured_width_ = 0;
  // Indicate the height after layout using |layout_width_|.
  int measured_height_ = 0;
};

class PreShadowLayoutContextText : public PreLayoutContext {
 public:
  PreShadowLayoutContextText() = default;
  virtual ~PreShadowLayoutContextText() = default;

  void CollectInlineImages(InlineImageShadowNode*);

  void CollectInlineViews(InlineViewShadowNode*);

  // Consume the collected inline images
  std::vector<InlineImageShadowNode*> TakeInlineImages();

  // Consume the collected inline views
  std::vector<InlineViewShadowNode*> TakeInlineViews();

 private:
  std::vector<InlineImageShadowNode*> inline_images_;
  std::vector<InlineViewShadowNode*> inline_views_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_SHADOW_LAYOUT_CONTEXT_H_
