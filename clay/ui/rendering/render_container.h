// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_CONTAINER_H_
#define CLAY_UI_RENDERING_RENDER_CONTAINER_H_

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

class RenderContainer : public RenderBox {
 public:
  RenderContainer() = default;
  ~RenderContainer() override = default;

  const char* GetName() const override;

  void Paint(PaintingContext& context, const FloatPoint& offset) override;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_CONTAINER_H_
