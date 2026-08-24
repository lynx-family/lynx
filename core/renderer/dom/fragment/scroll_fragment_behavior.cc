// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/scroll_fragment_behavior.h"

#include <algorithm>

#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx::tasm {

ScrollFragmentBehavior::ScrollFragmentBehavior(Fragment* fragment)
    : FragmentBehavior(fragment) {}

void ScrollFragmentBehavior::BeforeDrawChildren(
    DisplayListBuilder& display_list_builder) {
  if (fragment_) {
    display_list_builder.BeginScrollContent(fragment_->id(), GetType());
  }
}

void ScrollFragmentBehavior::AfterDrawChildren(
    DisplayListBuilder& display_list_builder) {
  if (fragment_) {
    const auto& layout_result = fragment_->LayoutResult().layout_result;
    const auto& padding = layout_result.padding_;
    float content_width = padding[starlight::kLeft];
    float content_height = padding[starlight::kTop];
    // TODO: consider about z-index children.
    for (Fragment* child : fragment_->children()) {
      if (child) {
        const auto& child_layout_result = child->LayoutResult().layout_result;
        const auto& child_offset = child_layout_result.offset_;
        const auto& child_size = child_layout_result.size_;
        const auto& child_margin = child_layout_result.margin_;
        float child_right = child_offset.X() + child_size.width_ +
                            child_margin[starlight::Direction::kRight];
        float child_bottom = child_offset.Y() + child_size.height_ +
                             child_margin[starlight::Direction::kBottom];
        content_width = std::max(content_width, child_right);
        content_height = std::max(content_height, child_bottom);
      }
    }
    content_width += padding[starlight::kRight];
    content_height += padding[starlight::kBottom];
    display_list_builder.EndScrollContent(content_width, content_height);
  }
}

}  // namespace lynx::tasm
