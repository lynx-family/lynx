// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/compositor/compositor_state.h"

#include <utility>

#include "base/trace/native/trace_event.h"

namespace clay {

CompositorState::CompositorState(skity::Vec2 frame_size)
    : frame_size_(frame_size) {}

CompositorState::~CompositorState() {}

void CompositorState::PrerollCompositeEmbeddedView(
    int view_id, std::unique_ptr<EmbeddedViewParams> params) {
  TRACE_EVENT("clay", "CompositorState::PrerollCompositeEmbeddedView");

  skity::Rect view_bounds = skity::Rect::MakeSize(frame_size_);
  std::unique_ptr<EmbedderViewSlice> view;
#ifndef ENABLE_SKITY
  view = std::make_unique<SkPictureEmbedderViewSlice>(view_bounds);
#else
  view = std::make_unique<SkityPictureEmbedderViewSlice>(view_bounds);
#endif  // ENABLE_SKITY
  composition_order_.push_back(view_id);
  slices_.insert_or_assign(view_id, std::move(view));
  view_params_[view_id] = std::move(params);
}

clay::GrCanvas* CompositorState::CompositeEmbeddedView(int view_id) {
  if (slices_.count(view_id) == 1) {
    return slices_.at(view_id)->canvas();
  }
  return nullptr;
}

void CompositorState::PushFilterToVisitedPlatformViews(
    const std::shared_ptr<const clay::ImageFilter>& filter,
    const skity::Rect& filter_rect) {
  FML_UNIMPLEMENTED();
}

void CompositorState::PrerollOverlayView(
    int view_id, std::unique_ptr<OverlayViewParams> params) {
  TRACE_EVENT("clay", "CompositorState::PrerollCompositeEmbeddedView");

  std::unique_ptr<EmbedderViewSlice> view;
#ifndef ENABLE_SKITY
  view = std::make_unique<SkPictureEmbedderViewSlice>(params->bounds());
#else
  view = std::make_unique<SkityPictureEmbedderViewSlice>(params->bounds());
#endif  // ENABLE_SKITY
  composition_order_.push_back(view_id);
  overlay_slices_.insert_or_assign(view_id, std::move(view));
  overlay_view_params_[view_id] = std::move(params);
}

clay::GrCanvas* CompositorState::CompositeOverlayView(int view_id) {
  if (overlay_slices_.count(view_id) == 1) {
    return overlay_slices_.at(view_id)->canvas();
  }
  return nullptr;
}

}  // namespace clay
