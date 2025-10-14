// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_COMPOSITOR_STATE_H_
#define CLAY_FLOW_COMPOSITOR_COMPOSITOR_STATE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "clay/common/service/service_manager.h"
#include "clay/flow/compositor/overlay_views.h"
#include "clay/flow/embedded_views.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class CompositorState {
 public:
  explicit CompositorState(skity::Vec2 frame_size);

  ~CompositorState();

  void PrerollCompositeEmbeddedView(int view_id,
                                    std::unique_ptr<EmbeddedViewParams> params);

  clay::GrCanvas* CompositeEmbeddedView(int view_id);

  void PrerollOverlayView(int view_id,
                          std::unique_ptr<OverlayViewParams> params);

  clay::GrCanvas* CompositeOverlayView(int view_id);

  void PushFilterToVisitedPlatformViews(
      const std::shared_ptr<const clay::ImageFilter>& filter,
      const skity::Rect& filter_rect);

  const skity::Vec2& GetFrameSize() const { return frame_size_; }

  std::vector<int64_t>& GetCompositionOrder() { return composition_order_; }

  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>>& GetSlices() {
    return slices_;
  }

  std::unordered_map<int64_t, std::unique_ptr<EmbeddedViewParams>>&
  GetViewParams() {
    return view_params_;
  }

 private:
  skity::Vec2 frame_size_;

  // The order of composition. Each entry contains a unique id for the platform
  // view or overlay view.
  std::vector<int64_t> composition_order_;

  // The |EmbedderViewSlice| implementation keyed off the platform view id,
  // which contains any subsequent operations until the next platform view or
  // the end of the last leaf node in the layer tree.
  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>> slices_;

  // The params for a platform view, which contains the size, position and
  // mutation stack.
  std::unordered_map<int64_t, std::unique_ptr<EmbeddedViewParams>> view_params_;

  // slices for overlay views
  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>>
      overlay_slices_;

  // The params for a overlay view, which contains the bounds rect.
  std::unordered_map<int64_t, std::unique_ptr<OverlayViewParams>>
      overlay_view_params_;
};

}  // namespace clay

#endif  // CLAY_FLOW_COMPOSITOR_COMPOSITOR_STATE_H_
