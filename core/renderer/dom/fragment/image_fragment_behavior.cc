// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/fragment/image_fragment_behavior.h"

#include "base/include/value/base_string.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"

namespace lynx::tasm {

namespace {

BASE_STATIC_STRING_DECL(kModeAspectFit, "aspectFit");
BASE_STATIC_STRING_DECL(kModeScaleToFill, "scaleToFill");
BASE_STATIC_STRING_DECL(kModeCenter, "center");

ImageFitMode ResolveImageFitMode(const base::String& mode) {
  if (mode.IsEqual(kModeAspectFit)) {
    return ImageFitMode::kAspectFit;
  }
  if (mode.IsEqual(kModeScaleToFill)) {
    return ImageFitMode::kScaleToFill;
  }
  if (mode.IsEqual(kModeCenter)) {
    return ImageFitMode::kCenter;
  }
  return ImageFitMode::kAspectFill;
}

}  // namespace

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
    paint_image_ = painting_context_->CreateImage(
        fragment_->id(), image_url_, layout_info.GetContentBoxWidth(),
        layout_info.GetContentBoxHeight(), event_mask_);
    fragment_->InvalidateForRedraw();
  }
}

void ImageFragmentBehavior::OnDraw(DisplayListBuilder& display_list_builder) {
  if (!paint_image_) {
    return;
  }
  const auto* image_element =
      static_cast<const ImageElement*>(fragment_->element());
  display_list_builder.DrawImage(
      paint_image_, fragment()->DefineContentBox(display_list_builder),
      ResolveImageFitMode(image_element->mode()));
}

}  // namespace lynx::tasm
