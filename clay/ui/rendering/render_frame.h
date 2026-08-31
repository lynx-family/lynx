// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_FRAME_H_
#define CLAY_UI_RENDERING_RENDER_FRAME_H_

#include <optional>

#include "clay/flow/frame_surface_registry.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

class RenderFrame : public RenderBox {
 public:
  RenderFrame() = default;
  ~RenderFrame() override = default;

  const char* GetName() const override;
  bool IsRepaintBoundary() const override { return true; }
  void Paint(PaintingContext& context, const FloatPoint& offset) override;
  void SetFrameSurfaceId(const FrameSurfaceId& surface_id) {
    surface_id_ = surface_id;
  }
  void ClearFrameSurfaceId() { surface_id_.reset(); }

 private:
  std::optional<FrameSurfaceId> surface_id_;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_FRAME_H_
