// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_frame.h"

#include <algorithm>

namespace clay {

const char* RenderFrame::GetName() const { return "RenderFrame"; }

void RenderFrame::Paint(PaintingContext& context, const FloatPoint& offset) {
  if (!CanDisplay()) {
    return;
  }

  RenderBox::Paint(context, offset);

  const FloatRect content_rect = ContentBoxRect();
  const float width = std::max(content_rect.width(), 0.f);
  const float height = std::max(content_rect.height(), 0.f);
  if (width <= 0.f || height <= 0.f) {
    return;
  }

  const auto rect =
      skity::Rect::MakeXYWH(offset.x() + content_rect.x(),
                            offset.y() + content_rect.y(), width, height);
  if (surface_id_) {
    context.AddFrameSurface(*surface_id_, rect);
    return;
  }
  context.AddFrameSurface(element_id(), rect);
}

}  // namespace clay
