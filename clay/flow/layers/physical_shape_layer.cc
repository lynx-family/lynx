// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/physical_shape_layer.h"

#include "clay/flow/paint_utils.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

namespace {
#ifndef ENABLE_SKITY
const SkScalar kLightHeight = 600;
const SkScalar kLightRadius = 800;

SkRect ComputeShadowBounds(const SkPath& path, float elevation, SkScalar dpr,
                           const SkMatrix& ctm) {
  SkRect shadow_bounds(path.getBounds());
  SkShadowUtils::GetLocalBounds(
      ctm, path, SkPoint3::Make(0, 0, dpr * elevation),
      SkPoint3::Make(0, -1, 1), kLightRadius / kLightHeight,
      SkShadowFlags::kDirectionalLight_ShadowFlag, &shadow_bounds);
  return shadow_bounds;
}

void DrawShadow(SkCanvas* canvas, const SkPath& path, clay::Color color,
                float elevation, bool transparentOccluder, SkScalar dpr) {
  const SkScalar kAmbientAlpha = 0.039f;
  const SkScalar kSpotAlpha = 0.25f;

  uint32_t flags = transparentOccluder
                       ? SkShadowFlags::kTransparentOccluder_ShadowFlag
                       : SkShadowFlags::kNone_ShadowFlag;
  flags |= SkShadowFlags::kDirectionalLight_ShadowFlag;
  SkColor in_ambient = SkColorSetA(color, kAmbientAlpha * SkColorGetA(color));
  SkColor in_spot = SkColorSetA(color, kSpotAlpha * SkColorGetA(color));
  SkColor ambient_color, spot_color;
  SkShadowUtils::ComputeTonalColors(in_ambient, in_spot, &ambient_color,
                                    &spot_color);
  SkShadowUtils::DrawShadow(canvas, path, SkPoint3::Make(0, 0, dpr * elevation),
                            SkPoint3::Make(0, -1, 1),
                            kLightRadius / kLightHeight, ambient_color,
                            spot_color, flags);
}
#endif  // ENABLE_SKITY
}  // namespace

PhysicalShapeLayer::PhysicalShapeLayer(clay::GrColor color,
                                       clay::GrColor shadow_color,
                                       float elevation,
                                       const clay::GrPath& path,
                                       Clip clip_behavior)
    : color_(color),
      shadow_color_(shadow_color),
      elevation_(elevation),
      path_(path),
      clip_behavior_(clip_behavior) {}

void PhysicalShapeLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const PhysicalShapeLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (color_ != prev->color_ || shadow_color_ != prev->shadow_color_ ||
        elevation_ != prev->elevation() || path_ != prev->path_ ||
        clip_behavior_ != prev->clip_behavior_) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }

  skity::Rect bounds;
#ifndef ENABLE_SKITY
  if (elevation_ == 0) {
    bounds = clay::ConvertSkRectToSkityRect(path_.getBounds());
  } else {
    bounds = clay::ConvertSkRectToSkityRect(ComputeShadowBounds(
        path_, elevation_, context->frame_device_pixel_ratio(),
        clay::ConvertSkityMatrixToSkMatrix(context->GetTransform())));
  }
#else
  if (elevation_ == 0) {
    auto skity_bounds = path_.GetBounds();
    bounds = skity::Rect::MakeXYWH(skity_bounds.X(), skity_bounds.Y(),
                                   skity_bounds.Width(), skity_bounds.Height());
  } else {
    // TODO(zhangxiao.ninja) implement shadow bounds later
    FML_DCHECK(false);
  }
#endif  // ENABLE_SKITY

  context->AddLayerBounds(bounds);

  // Only push cull rect if there is clip.
  if (clip_behavior_ == Clip::none || context->PushCullRect(bounds)) {
    DiffChildren(context, prev);
  }
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void PhysicalShapeLayer::Preroll(PrerollContext* context) {
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context, UsesSaveLayer());

  skity::Rect child_paint_bounds = skity::Rect::MakeEmpty();
  PrerollChildren(context, &child_paint_bounds);
  context->renderable_state_flags =
      UsesSaveLayer() ? Layer::kSaveLayerRenderFlags : 0;

  skity::Rect paint_bounds;
#ifndef ENABLE_SKITY
  if (elevation_ == 0) {
    paint_bounds = clay::ConvertSkRectToSkityRect(path_.getBounds());
  } else {
    // We will draw the shadow in Paint(), so add some margin to the paint
    // bounds to leave space for the shadow.
    paint_bounds = clay::ConvertSkRectToSkityRect(ComputeShadowBounds(
        path_, elevation_, context->frame_device_pixel_ratio,
        clay::ConvertSkityMatrixToSkMatrix(
            context->state_stack.transform_4x4())));
  }
#else
  if (elevation_ == 0) {
    auto skity_bounds = path_.GetBounds();
    paint_bounds =
        skity::Rect::MakeXYWH(skity_bounds.X(), skity_bounds.Y(),
                              skity_bounds.Width(), skity_bounds.Height());
  } else {
    // TODO(zhangxiao.ninja) implement shadow bounds later
    FML_DCHECK(false);
  }
#endif  // ENABLE_SKITY

  if (clip_behavior_ == Clip::none) {
    paint_bounds.Join(child_paint_bounds);
  }

  set_paint_bounds(paint_bounds);
}

void PhysicalShapeLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));

#ifndef ENABLE_SKITY
  if (elevation_ != 0) {
    DrawShadow(context.canvas, path_, shadow_color_, elevation_,
               SkColorGetA(color_) != 0xff, context.frame_device_pixel_ratio);
  }
#endif  // ENABLE_SKITY

  // Call drawPath without clip if possible for better performance.
  clay::GrPaint paint;
  PAINT_SET_COLOR(paint, color_);
  PAINT_SET_ANTI_ALIAS(paint, true);
  if (clip_behavior_ != Clip::antiAliasWithSaveLayer) {
    CANVAS_DRAW_PATH(context.canvas, path_, paint);
  }

  auto mutator = context.state_stack.save();
  switch (clip_behavior_) {
    case Clip::hardEdge:
      mutator.clipPath(path_, false);
      break;
    case Clip::antiAlias:
      mutator.clipPath(path_, true);
      break;
    case Clip::antiAliasWithSaveLayer: {
      TRACE_EVENT("clay", "Canvas::saveLayer");
      mutator.clipPath(path_, true);
      mutator.saveLayer(paint_bounds());
    } break;
    case Clip::none:
      break;
  }

  if (UsesSaveLayer()) {
    // If we want to avoid the bleeding edge artifact
    // (https://github.com/flutter/flutter/issues/18057#issue-328003931)
    // using saveLayer, we have to call drawPaint instead of drawPath as
    // anti-aliased drawPath will always have such artifacts.
    CANVAS_DRAW_PAINT(context.canvas, paint);
  }

  PaintChildren(context);
}

}  // namespace clay
