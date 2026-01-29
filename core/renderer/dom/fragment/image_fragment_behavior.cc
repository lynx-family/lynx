// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/image_fragment_behavior.h"

#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/starlight/types/layout_result.h"

namespace lynx::tasm {

void ImageFragmentBehavior::OnUpdateLayout(
    const LayoutInfoForDraw& layout_info) {
  const auto& current_src =
      static_cast<ImageElement*>(fragment_->element())->src();

  if (image_url_ != current_src) {
    image_url_ = current_src;
    painting_context_->CreateImage(fragment_->id(), image_url_,
                                   layout_info.GetContentBoxWidth(),
                                   layout_info.GetContentBoxHeight());
  }
}

void ImageFragmentBehavior::OnDraw(DisplayListBuilder& display_list_builder) {
  constexpr const static int32_t kInvalidIndex = -1;

  display_list_builder.DrawImage(
      fragment()->id(),
      fragment()->LayoutResult().border_radius_info != std::nullopt
          ? fragment()->DefineContentBox(display_list_builder)
          : kInvalidIndex);
}

}  // namespace lynx::tasm
