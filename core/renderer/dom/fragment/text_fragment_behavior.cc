// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/text_fragment_behavior.h"

#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx::tasm {

void TextFragmentBehavior::CreatePlatformRenderer() {
  if (painting_context_ && fragment_) {
    painting_context_->CreatePlatformRenderer(fragment_->id(),
                                              PlatformRendererType::kText);
  }
}

void TextFragmentBehavior::OnDraw(DisplayListBuilder& builder) {
  const auto& layout_result = fragment_->LayoutResult();
  builder.Begin(layout_result.padding_[starlight::Direction::kLeft],
                layout_result.padding_[starlight::Direction::kTop],
                layout_result.size_.width_, layout_result.size_.height_);
  builder.DrawText(fragment_->id());
  builder.End();
}

}  // namespace lynx::tasm
