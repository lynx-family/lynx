// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_PAGE_H_
#define CLAY_UI_RENDERING_RENDER_PAGE_H_

#include <memory>
#include <string>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/compositing/pending_transform_layer.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

// The root of the render tree.
class RenderPage : public RenderBox {
 public:
  RenderPage();
  ~RenderPage() override;

  const char* GetName() const override;
  bool IsRepaintBoundary() const override { return true; }
  void Paint(PaintingContext& context, const FloatPoint& offset) override;
  void SetScaleRatio(float ratio);

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  // This is the clay to physical pixel ratio. We scale the root layer by dpr if
  // logical pixels are used in clay.
  float scale_ratio_ = 1.0f;
  std::unique_ptr<PendingTransformLayer> CreateNewRootLayer();
};

}  // namespace clay
#endif  // CLAY_UI_RENDERING_RENDER_PAGE_H_
