// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/frame_child_surface_host.h"

#include <utility>

#include "clay/ui/component/page_view.h"

namespace clay {

FrameChildSurfaceHost::FrameChildSurfaceHost(const ElementId& element_id,
                                             PageView* page_view)
    : element_id_(element_id),
      page_view_(page_view),
      frame_surface_registry_(page_view ? page_view->GetFrameSurfaceRegistry()
                                        : nullptr) {}

FrameChildSurfaceHost::~FrameChildSurfaceHost() { Clear(); }

std::optional<FrameSurfaceId> FrameChildSurfaceHost::SubmitLayerTree(
    std::shared_ptr<LayerTree> layer_tree, const FloatSize& surface_size,
    std::optional<skity::Rect> damage_rect) {
  SetSurfaceSize(surface_size);
  if (!frame_surface_registry_) {
    return std::nullopt;
  }
  FrameSurfaceId surface_id(element_id_, incarnation_, ++generation_,
                            size_generation_);
  if (!frame_surface_registry_->SubmitPendingSurface(
          surface_id, std::move(layer_tree), surface_size_, damage_rect)) {
    return std::nullopt;
  }
  return surface_id;
}

void FrameChildSurfaceHost::SetSurfaceSize(const FloatSize& surface_size) {
  if (surface_size_ == surface_size) {
    return;
  }
  surface_size_ = surface_size;
  ++size_generation_;
  // Tell the registry the version we now expect. Any pending surface whose
  // `size_generation` is smaller than this value will not be promoted to
  // active until the child submits a matching (or newer) version. This is
  // the version-alignment gate that eliminates the resize race.
  if (frame_surface_registry_) {
    frame_surface_registry_->SetExpectedSizeGeneration(element_id_,
                                                       size_generation_);
  }
}

void FrameChildSurfaceHost::Clear() {
  if (frame_surface_registry_) {
    frame_surface_registry_->RemoveSurfacesForElement(element_id_);
  }
  // Generation may restart after a reload, so move to a new producer identity
  // before the next surface is submitted.
  ++incarnation_;
  if (incarnation_ == 0) {
    ++incarnation_;
  }
  generation_ = 0;
  size_generation_ = 0;
  surface_size_ = FloatSize();
  if (page_view_) {
    page_view_->RequestPaintBase();
  }
}

}  // namespace clay
