// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_FRAME_CHILD_SURFACE_HOST_H_
#define CLAY_UI_COMPONENT_FRAME_CHILD_SURFACE_HOST_H_

#include <cstdint>
#include <memory>
#include <optional>

#include "clay/common/element_id.h"
#include "clay/flow/frame_surface_registry.h"
#include "clay/gfx/geometry/float_size.h"

namespace clay {

class LayerTree;
class PageView;

class FrameChildSurfaceHost {
 public:
  FrameChildSurfaceHost(const ElementId& element_id, PageView* page_view);
  ~FrameChildSurfaceHost();

  FrameChildSurfaceHost(const FrameChildSurfaceHost&) = delete;
  FrameChildSurfaceHost& operator=(const FrameChildSurfaceHost&) = delete;

  std::optional<FrameSurfaceId> SubmitLayerTree(
      std::shared_ptr<LayerTree> layer_tree, const FloatSize& surface_size,
      std::optional<skity::Rect> damage_rect);
  void SetSurfaceSize(const FloatSize& surface_size);
  void Clear();

  const ElementId& element_id() const { return element_id_; }
  uint64_t incarnation() const { return incarnation_; }
  uint64_t generation() const { return generation_; }
  uint64_t size_generation() const { return size_generation_; }

 private:
  ElementId element_id_;
  PageView* page_view_ = nullptr;
  std::shared_ptr<FrameSurfaceRegistry> frame_surface_registry_;
  uint64_t incarnation_ = 1;
  uint64_t generation_ = 0;
  uint64_t size_generation_ = 0;
  FloatSize surface_size_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_FRAME_CHILD_SURFACE_HOST_H_
