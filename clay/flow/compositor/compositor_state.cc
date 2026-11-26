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

std::unique_ptr<CompositorState> CompositorState::CloneForPresentation() {
  auto state = std::make_unique<CompositorState>(frame_size_);
  for (const auto& entry : slices_) {
    entry.second->end_recording();
  }
  state->slices_ = slices_;
  state->has_finished_slices_ = true;
  state->display_scene_ = display_scene_;
  state->composition_order_ = composition_order_;
  for (const auto& entry : view_params_) {
    state->view_params_[entry.first] =
        std::make_unique<EmbeddedViewParams>(*entry.second);
  }
  return state;
}

void CompositorState::PrerollCompositeEmbeddedView(
    int view_id, std::unique_ptr<EmbeddedViewParams> params) {
  TRACE_EVENT("clay", "CompositorState::PrerollCompositeEmbeddedView");

  skity::Rect view_bounds =
      params->NeedsOverlayLayer()
          ? params->finalBoundingRect()
          : skity::Rect::MakeWH(frame_size_.x, frame_size_.y);

  std::shared_ptr<EmbedderViewSlice> view;
#ifndef ENABLE_SKITY
  view = std::make_shared<SkPictureEmbedderViewSlice>(view_bounds);
#else
  view = std::make_shared<SkityPictureEmbedderViewSlice>(view_bounds);
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

}  // namespace clay
