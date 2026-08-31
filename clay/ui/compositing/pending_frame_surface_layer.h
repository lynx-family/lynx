// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_FRAME_SURFACE_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_FRAME_SURFACE_LAYER_H_

#include <optional>
#include <string>

#include "clay/common/element_id.h"
#include "clay/flow/frame_surface_registry.h"
#include "clay/ui/compositing/pending_layer.h"

namespace clay {

class PendingFrameSurfaceLayer : public PendingLayer {
 public:
  PendingFrameSurfaceLayer(const ElementId& element_id,
                           const skity::Rect& rect);
  PendingFrameSurfaceLayer(const FrameSurfaceId& surface_id,
                           const skity::Rect& rect);
  ~PendingFrameSurfaceLayer() override;

  std::string GetName() const override { return "PendingFrameSurfaceLayer"; }

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;

  ElementId element_id_;
  std::optional<FrameSurfaceId> surface_id_;
  skity::Rect rect_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_FRAME_SURFACE_LAYER_H_
