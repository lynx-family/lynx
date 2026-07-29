// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/fragment/image_fragment_behavior.h"

#include "base/include/value/base_string.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx::tasm {

namespace {

constexpr int32_t kImageModeScaleToFill = 0;
constexpr int32_t kImageModeAspectFit = 1;
constexpr int32_t kImageModeAspectFill = 2;
constexpr int32_t kImageModeCenter = 3;

BASE_STATIC_STRING_DECL(kModeAspectFit, "aspectFit");
BASE_STATIC_STRING_DECL(kModeAspectFill, "aspectFill");
BASE_STATIC_STRING_DECL(kModeCenter, "center");

}  // namespace

int32_t ImageFragmentBehavior::ResolveImageMode(const base::String& mode) {
  if (mode.IsEqual(kModeAspectFit)) {
    return kImageModeAspectFit;
  }
  if (mode.IsEqual(kModeAspectFill)) {
    return kImageModeAspectFill;
  }
  if (mode.IsEqual(kModeCenter)) {
    return kImageModeCenter;
  }
  return kImageModeScaleToFill;
}

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

  if (UpdateImageIfNeeded(fragment_->LayoutResult())) {
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
  const int32_t current_mode = ResolveImageMode(image_element->mode());
  const float current_width = layout_info.GetContentBoxWidth();
  const float current_height = layout_info.GetContentBoxHeight();

  if (image_url_ == current_src &&
      (current_src.empty() ||
       (image_mode_ == current_mode && image_width_ == current_width &&
        image_height_ == current_height))) {
    return false;
  }

  if (event_mask_ < 0) {
    event_mask_ = ComputeEventMask();
  }
  painting_context_->CreateImage(fragment_->id(), current_src, current_mode,
                                 current_width, current_height, event_mask_);
  image_url_ = current_src;
  image_mode_ = current_mode;
  image_width_ = current_width;
  image_height_ = current_height;
  return true;
}

void ImageFragmentBehavior::OnDraw(DisplayListBuilder& display_list_builder) {
  display_list_builder.DrawImage(
      fragment()->id(), fragment()->DefineContentBox(display_list_builder));
}

}  // namespace lynx::tasm
