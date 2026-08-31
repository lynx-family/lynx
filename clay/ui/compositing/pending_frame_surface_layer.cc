// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_frame_surface_layer.h"

#include <sstream>

#include "clay/ui/compositing/frame_builder.h"

namespace clay {

PendingFrameSurfaceLayer::PendingFrameSurfaceLayer(const ElementId& element_id,
                                                   const skity::Rect& rect)
    : element_id_(element_id), rect_(rect) {}

PendingFrameSurfaceLayer::PendingFrameSurfaceLayer(
    const FrameSurfaceId& surface_id, const skity::Rect& rect)
    : element_id_(surface_id.element_id()),
      surface_id_(surface_id),
      rect_(rect) {}

PendingFrameSurfaceLayer::~PendingFrameSurfaceLayer() = default;

void PendingFrameSurfaceLayer::AddToFrame(FrameBuilder* builder,
                                          const FloatPoint& offset) {
  const auto rect = skity::Rect::MakeLTRB(
      rect_.Left() + offset.x(), rect_.Top() + offset.y(),
      rect_.Right() + offset.x(), rect_.Bottom() + offset.y());
  if (surface_id_) {
    builder->AddFrameSurface(*surface_id_, rect);
    return;
  }
  builder->AddFrameSurface(element_id_, rect);
}

#ifndef NDEBUG
std::string PendingFrameSurfaceLayer::ToString() const {
  std::stringstream ss;
  ss << PendingLayer::ToString();
  ss << " frame_id=" << element_id_.view_id();
  if (surface_id_) {
    ss << " surface_incarnation=" << surface_id_->incarnation()
       << " surface_generation=" << surface_id_->generation()
       << " size_generation=" << surface_id_->size_generation();
  }
  ss << " rect=(" << rect_.Left() << "," << rect_.Top() << "," << rect_.Right()
     << "," << rect_.Bottom() << ")";
  return ss.str();
}
#endif

}  // namespace clay
