// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/shadow_layout_context.h"

#include <utility>

namespace clay {

void PreShadowLayoutContextText::CollectInlineImages(
    InlineImageShadowNode* image) {
  inline_images_.emplace_back(image);
}

void PreShadowLayoutContextText::CollectInlineViews(
    InlineViewShadowNode* view) {
  inline_views_.emplace_back(view);
}

std::vector<InlineImageShadowNode*>
PreShadowLayoutContextText::TakeInlineImages() {
  return std::move(inline_images_);
}

std::vector<InlineViewShadowNode*>
PreShadowLayoutContextText::TakeInlineViews() {
  return std::move(inline_views_);
}

}  // namespace clay
