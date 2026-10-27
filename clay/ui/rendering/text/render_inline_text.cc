// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/text/render_inline_text.h"

#include "clay/ui/painter/box_painter.h"
#include "clay/ui/painter/box_shadow_painter.h"

namespace clay {

void RenderInlineText::Paint(PaintingContext& context,
                             const FloatPoint& offset) {
  auto box_painter = BoxPainter(this);

  // Merge boxes in same line to one box.
  std::list<FloatRect> merged_box;
  for (const auto& box : text_boxes_) {
    if (!merged_box.empty() && merged_box.back().y() == box.y()) {
      merged_box.back().ShiftMaxXEdgeTo(box.MaxX());
    } else {
      merged_box.push_back(box);
    }
  }

  for (auto box : merged_box) {
    box.Expand(PaddingTop(), PaddingRight(), PaddingBottom(), PaddingLeft());
    box_painter.SetOuterRect(box);
    if (HasOutline()) {
      box_painter.SetOutline(Outline(), box);
    }
    if (HasBorder()) {
      box_painter.SetBorder(Border(), box);
    }
    box_painter.PaintBoxDecorationBackground(context.GetGraphicsContext(),
                                             offset, box);
  }
  RenderBox::PaintChildren(context, offset);
}

}  // namespace clay
