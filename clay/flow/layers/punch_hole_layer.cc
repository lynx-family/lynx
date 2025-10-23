// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/punch_hole_layer.h"

#include "clay/gfx/rendering_backend.h"

namespace clay {

PunchHoleLayer::PunchHoleLayer(const skity::Rect& punch_hole)
    : punch_hole_(punch_hole) {}

void PunchHoleLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(old_layer);
    auto prev = old_layer->as_punch_hole_layer();
    if (punch_hole_ != prev->PunchHoleRect()) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(prev));
    }
  }
  context->AddLayerBounds(punch_hole_);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void PunchHoleLayer::Preroll(PrerollContext* context) {
  context->has_punch_hole_layer = true;
  context->renderable_state_flags = LayerStateStack::kCallerCanApplyOpacity;
  set_paint_bounds(punch_hole_);
  set_subtree_has_punch_hole(true);
}

void PunchHoleLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));
  if (punch_hole_.IsEmpty()) {
    return;
  }
  clay::GrAutoCanvasRestore save_restore(context.canvas, true);
  clay::GrPaint paint;
  context.state_stack.fill(paint);
  PAINT_SET_STYLE(paint, clay::GrPaint::kFill_Style);
  PAINT_SET_BLEND_MODE(paint, clay::BlendMode::kClear);
  CANVAS_DRAW_RECT(context.canvas, punch_hole_, paint);
}
}  // namespace clay
