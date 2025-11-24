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
    const starlight::LayoutResultForRendering& layout_result) {
  const char* current_src =
      static_cast<ImageElement*>(fragment_->element())->src();
  if (image_url_ != current_src) {
    image_url_ = current_src;
    painting_context_->CreateImage(fragment_->id(), image_url_,
                                   layout_result.size_.width_,
                                   layout_result.size_.height_);
  }
}

void ImageFragmentBehavior::OnDraw(DisplayListBuilder& display_list_builder) {
  display_list_builder.DrawImage(fragment_->id());
}

}  // namespace lynx::tasm
