// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_COMPOSITOR_STATE_H_
#define CLAY_FLOW_COMPOSITOR_COMPOSITOR_STATE_H_

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/common/service/service_manager.h"
#include "clay/flow/embedded_views.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class DisplayScene;

class CompositorState {
 public:
  explicit CompositorState(skity::Vec2 frame_size);

  ~CompositorState();

  // Finish recording and retain the slices for subsequent display frames.
  // Presentation consumes its own copy of the view parameters and order.
  std::unique_ptr<CompositorState> CloneForPresentation();

  void PrerollCompositeEmbeddedView(int view_id,
                                    std::unique_ptr<EmbeddedViewParams> params);
  clay::GrCanvas* CompositeEmbeddedView(int view_id);

  void PushFilterToVisitedPlatformViews(
      const std::shared_ptr<const clay::ImageFilter>& filter,
      const skity::Rect& filter_rect);

  void SetRecordDisplayScene(bool enabled) { record_display_scene_ = enabled; }
  bool RecordsDisplayScene() const { return record_display_scene_; }
  void SetDisplayScene(std::shared_ptr<DisplayScene> scene) {
    display_scene_ = std::move(scene);
  }
  const std::shared_ptr<DisplayScene>& GetDisplayScene() const {
    return display_scene_;
  }

  bool HasFinishedSlices() const { return has_finished_slices_; }

  const skity::Vec2& GetFrameSize() const { return frame_size_; }

  std::vector<int64_t>& GetCompositionOrder() { return composition_order_; }

  std::unordered_map<int64_t, std::shared_ptr<EmbedderViewSlice>>& GetSlices() {
    return slices_;
  }

  std::unordered_map<int64_t, std::unique_ptr<EmbeddedViewParams>>&
  GetViewParams() {
    return view_params_;
  }

 private:
  skity::Vec2 frame_size_;
  bool has_finished_slices_ = false;
  bool record_display_scene_ = false;
  std::shared_ptr<DisplayScene> display_scene_;

  // The order of composition. Each entry contains a unique id for a composited
  // entry.
  std::vector<int64_t> composition_order_;

  // The |EmbedderViewSlice| implementation keyed off the platform view id,
  // which contains any subsequent operations until the next platform view or
  // the end of the last leaf node in the layer tree.
  std::unordered_map<int64_t, std::shared_ptr<EmbedderViewSlice>> slices_;

  // The params for a composited entry, including size, position, mutation
  // stack, and presentation kind.
  std::unordered_map<int64_t, std::unique_ptr<EmbeddedViewParams>> view_params_;
};

}  // namespace clay

#endif  // CLAY_FLOW_COMPOSITOR_COMPOSITOR_STATE_H_
