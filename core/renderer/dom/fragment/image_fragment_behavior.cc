// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/fragment/image_fragment_behavior.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"

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
  if (UpdateImageIfNeeded(layout_info)) {
    fragment_->InvalidateForRedraw();
  }
}

void ImageFragmentBehavior::OnAttributeUpdate(const fml::RefPtr<PropBundle>&) {
  if (!fragment_ || !fragment_->element()) {
    return;
  }

  const auto* image_element = static_cast<ImageElement*>(fragment_->element());
  const auto& current_src = image_element->src();
  if (image_url_ != current_src ||
      ((!current_src.empty() ||
        !image_element->paint_info().placeholder.empty() ||
        !image_paint_info_.placeholder.empty()) &&
       image_paint_info_ != image_element->paint_info())) {
    fragment_->InvalidateForRedraw();
  }
}

bool ImageFragmentBehavior::UpdateImageIfNeeded(
    const LayoutInfoForDraw& layout_info) {
  if (!painting_context_ || !fragment_ || !fragment_->element()) {
    return false;
  }

  const auto* image_element = static_cast<ImageElement*>(fragment_->element());
  const auto& current_src = image_element->src();
  const auto& current_paint_info = image_element->paint_info();
  const float current_width = layout_info.GetContentBoxWidth();
  const float current_height = layout_info.GetContentBoxHeight();
  const double pixels_per_unit = image_element->element_manager()
                                     ->GetLynxEnvConfig()
                                     .PhysicalPixelsPerLayoutUnit();
  // Compare against the last requested size so successive small changes can
  // still trigger a reload. Crossing zero must not suppress the first load.
  const auto same_dimension = [pixels_per_unit](float requested,
                                                float current) {
    if (requested == current) {
      return true;
    }
    // Layout rounding can represent one physical pixel slightly above one,
    // notably at scale 3. Allow only the precision lost by the float sizes.
    const double rounding_error =
        std::numeric_limits<float>::epsilon() *
        std::max(std::abs(requested), std::abs(current));
    return (requested > 0) == (current > 0) &&
           (std::abs(static_cast<double>(requested) - current) -
            rounding_error) *
                   pixels_per_unit <=
               1.;
  };
  const bool same_width = same_dimension(image_width_, current_width);
  const bool same_height = same_dimension(image_height_, current_height);

  if (image_url_ == current_src) {
    if (current_src.empty() && current_paint_info.placeholder.empty() &&
        image_paint_info_.placeholder.empty()) {
      return false;
    }
    if (image_paint_info_ == current_paint_info) {
      if (same_width && same_height) {
        return false;
      }
      // Keep the last requested size: shrinking and growing back within it
      // must not reload a downsampled image. Draw with the current layout.
      if (paint_image_ && !current_paint_info.auto_size && image_width_ > 0 &&
          image_height_ > 0 && current_width > 0 && current_height > 0 &&
          (!current_paint_info.downsampling ||
           ((current_width <= image_width_ || same_width) &&
            (current_height <= image_height_ || same_height))) &&
          paint_image_->CanReuseOnResize()) {
        return false;
      }
    }
  }

  if (event_mask_ < 0) {
    event_mask_ = ComputeEventMask();
  }
  auto paint_image = painting_context_->CreateImage(
      fragment_->id(), current_src, current_paint_info, current_width,
      current_height, event_mask_);
  if (!paint_image) {
    return false;
  }
  image_url_ = current_src;
  image_paint_info_ = current_paint_info;
  image_width_ = current_width;
  image_height_ = current_height;
  paint_image_ = paint_image;
  return true;
}

void ImageFragmentBehavior::OnDraw(DisplayListBuilder& display_list_builder) {
  UpdateImageIfNeeded(fragment_->LayoutResult());
  if (!paint_image_) {
    return;
  }
  display_list_builder.DrawImage(
      paint_image_, fragment()->DefineContentBox(display_list_builder));
}

}  // namespace lynx::tasm
