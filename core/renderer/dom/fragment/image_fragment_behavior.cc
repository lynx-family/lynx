// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/fragment/image_fragment_behavior.h"

#include "base/include/value/base_string.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx::tasm {

int32_t ImageFragmentBehavior::ComputeEventMask() const {
  int32_t event_mask = 0;
  auto* element = fragment_->element();
  if (!element) {
    return event_mask;
  }

  BASE_STATIC_STRING_DECL(kLoadEvent, "load");
  BASE_STATIC_STRING_DECL(kErrorEvent, "error");

  if (element->HasEventListener(kLoadEvent.str())) {
    event_mask |= kFlagImageLoadEvent;
  }

  if (element->HasEventListener(kErrorEvent.str())) {
    event_mask |= kFlagImageErrorEvent;
  }

  return event_mask;
}

void ImageFragmentBehavior::OnUpdateLayout(
    const LayoutInfoForDraw& layout_info) {
  const auto& current_src =
      static_cast<ImageElement*>(fragment_->element())->src();

  if (image_url_ != current_src) {
    image_url_ = current_src;
    // Lazily compute event mask on first use, then cache it.
    if (event_mask_ < 0) {
      event_mask_ = ComputeEventMask();
    }
    painting_context_->CreateImage(
        fragment_->id(), image_url_, layout_info.GetContentBoxWidth(),
        layout_info.GetContentBoxHeight(), event_mask_);
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
