// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/mock_layer.h"

#include <utility>

#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer.h"
#include "clay/flow/testing/mock_raster_cache.h"
#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {
namespace testing {

MockLayer::MockLayer(const SkPath& path, SkPaint paint)
    : fake_paint_path_(path), fake_paint_(std::move(paint)) {}

bool MockLayer::IsReplacing(DiffContext* context, const Layer* layer) const {
  // Similar to PictureLayer, only return true for identical mock layers;
  // That way ContainerLayer::DiffChildren can properly detect mock layer
  // insertion
  auto mock_layer = layer->as_mock_layer();
  return mock_layer && mock_layer->fake_paint_ == fake_paint_ &&
         mock_layer->fake_paint_path_ == fake_paint_path_;
}

void MockLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  context->AddLayerBounds(
      clay::ConvertSkRectToSkityRect(fake_paint_path_.getBounds()));
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void MockLayer::Preroll(PrerollContext* context) {
  context->state_stack.fill(&parent_mutators_);
  parent_matrix_ = context->state_stack.transform_4x4();
  parent_cull_rect_ = context->state_stack.local_cull_rect();

  set_parent_has_platform_view(context->has_platform_view);
  set_parent_has_drawable_image_layer(context->has_drawable_image_layer);
  set_parent_has_punch_hole_layer(context->has_punch_hole_layer);

  context->has_platform_view = fake_has_platform_view();
  context->has_drawable_image_layer = fake_has_drawable_image_layer();
  context->has_punch_hole_layer = fake_has_punch_hole_layer();
  set_paint_bounds(
      clay::ConvertSkRectToSkityRect(fake_paint_path_.getBounds()));
  if (fake_reads_surface()) {
    context->surface_needs_readback = true;
  }
  if (fake_opacity_compatible()) {
    context->renderable_state_flags = LayerStateStack::kCallerCanApplyOpacity;
  }
}

void MockLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));

  if (expected_paint_matrix_.has_value()) {
    skity::Matrix matrix =
        clay::ConvertSkMatrixToSkityMatrix(context.canvas->getTotalMatrix());

    EXPECT_EQ(matrix, expected_paint_matrix_.value());
  }

  SkPaint sk_paint = fake_paint_;
  context.state_stack.fill(sk_paint);
  context.canvas->drawPath(fake_paint_path_, sk_paint);
}

void MockCacheableContainerLayer::Preroll(PrerollContext* context) {
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);
  auto cache = AutoCache(layer_raster_cache_item_.get(), context,
                         context->state_stack.transform_4x4());

  ContainerLayer::Preroll(context);
}

void MockCacheableLayer::Preroll(PrerollContext* context) {
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);
  auto cache = AutoCache(raster_cache_item_.get(), context,
                         context->state_stack.transform_4x4());

  MockLayer::Preroll(context);
}

}  // namespace testing
}  // namespace clay
