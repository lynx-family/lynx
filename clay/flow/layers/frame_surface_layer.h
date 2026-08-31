// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_FRAME_SURFACE_LAYER_H_
#define CLAY_FLOW_LAYERS_FRAME_SURFACE_LAYER_H_

#include <memory>
#include <string>

#include "clay/flow/frame_surface_registry.h"
#include "clay/flow/layers/layer.h"

namespace clay {

// A SurfaceDrawQuad-like placeholder in the parent layer tree. The layer holds
// an acquired child FrameSurface for the lifetime of this parent LayerTree and
// aggregates its structured LayerTree into the current compositor state. This
// keeps platform views/HC as structure instead of flattening the child frame
// into a texture.
class FrameSurfaceLayer : public Layer {
 public:
  FrameSurfaceLayer(std::shared_ptr<FrameSurface> surface,
                    const skity::Rect& rect);
  ~FrameSurfaceLayer() override;

  bool IsReplacing(DiffContext* context, const Layer* old_layer) const override;
  void Diff(DiffContext* context, const Layer* old_layer) override;
  void Preroll(PrerollContext* context) override;
  void Paint(PaintContext& context) const override;

  const FrameSurfaceId& surface_id() const { return surface_->surface_id(); }
  const skity::Rect& rect() const { return rect_; }
  const FrameSurfaceLayer* as_frame_surface_layer() const override {
    return this;
  }

#ifndef NDEBUG
  std::string DebugName() const override { return "FrameSurfaceLayer"; }
#endif

 private:
  bool AddMappedSurfaceDamage(DiffContext* context) const;
  skity::Rect LocalSurfaceRect() const;

  std::shared_ptr<FrameSurface> surface_;
  skity::Rect rect_;
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_FRAME_SURFACE_LAYER_H_
