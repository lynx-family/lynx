// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_DUMMY_H_
#define CLAY_UI_RENDERING_RENDER_DUMMY_H_

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/rendering/render_object.h"

namespace clay {

// RenderDummy does NOT paint anything but it is useful to propagate the
// MarkNeedsPaint() to ancestor.
class RenderDummy : public RenderObject {
 public:
  RenderDummy() = default;
  virtual ~RenderDummy() = default;

  const char* GetName() const override { return "Dummy"; }

  void Paint(PaintingContext& context, const FloatPoint& offset) override {}
  void PaintChildren(PaintingContext& context,
                     const FloatPoint& offset) override {}
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_DUMMY_H_
