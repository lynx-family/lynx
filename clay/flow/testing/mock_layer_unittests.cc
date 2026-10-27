// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/mock_canvas.h"

namespace clay {
namespace testing {

using MockLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(MockLayerTest, PaintBeforePrerollDies) {
  SkPath path = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  auto layer = std::make_shared<MockLayer>(path, SkPaint());

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(MockLayerTest, PaintingEmptyLayerDies) {
  auto layer = std::make_shared<MockLayer>(SkPath(), SkPaint());

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(SkPath().getBounds()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(MockLayerTest, SimpleParams) {
  const SkPath path = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPaint paint = SkPaint(SkColors::kBlue);
  const skity::Matrix start_matrix = skity::Matrix::Translate(1.0f, 2.0f);
  const skity::Matrix scale_matrix = skity::Matrix::Scale(0.5f, 0.5f);
  const skity::Matrix combined_matrix = start_matrix * scale_matrix;
  const skity::Rect local_cull_rect = skity::Rect::MakeWH(5.0f, 5.0f);
  const skity::Rect device_cull_rect = combined_matrix.MapRect(local_cull_rect);
  const bool parent_has_platform_view = true;
  auto layer = std::make_shared<MockLayer>(path, paint);

  preroll_context()->state_stack.set_preroll_delegate(device_cull_rect,
                                                      start_matrix);
  auto mutator = preroll_context()->state_stack.save();
  mutator.transform(scale_matrix);
  preroll_context()->has_platform_view = parent_has_platform_view;
  layer->Preroll(preroll_context());
  EXPECT_EQ(preroll_context()->has_platform_view, false);
  EXPECT_EQ(layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(path.getBounds()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(layer->parent_mutators(), std::vector{Mutator(scale_matrix)});
  EXPECT_EQ(layer->parent_matrix(), combined_matrix);
  EXPECT_EQ(layer->parent_cull_rect(), local_cull_rect);
  EXPECT_EQ(layer->parent_has_platform_view(), parent_has_platform_view);

  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({MockCanvas::DrawCall{
                0, MockCanvas::DrawPathData{path, paint}}}));
}

TEST_F(MockLayerTest, FakePlatformView) {
  auto layer = std::make_shared<MockLayer>(SkPath(), SkPaint());
  layer->set_fake_has_platform_view(true);
  EXPECT_EQ(preroll_context()->has_platform_view, false);

  layer->Preroll(preroll_context());
  EXPECT_EQ(preroll_context()->has_platform_view, true);
}

TEST_F(MockLayerTest, SaveLayerOnLeafNodesCanvas) {
  auto layer = std::make_shared<MockLayer>(SkPath(), SkPaint());
  layer->set_fake_has_platform_view(true);
  EXPECT_EQ(preroll_context()->has_platform_view, false);

  layer->Preroll(preroll_context());
  EXPECT_EQ(preroll_context()->has_platform_view, true);
}

TEST_F(MockLayerTest, OpacityInheritance) {
  auto path1 = SkPath().addRect({10, 10, 30, 30});
  PrerollContext* context = preroll_context();

  auto mock1 = std::make_shared<MockLayer>(path1);
  mock1->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags, 0);

  auto mock2 = MockLayer::MakeOpacityCompatible(path1);
  mock2->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
}

TEST_F(MockLayerTest, FlagGetSet) {
  auto mock_layer = std::make_shared<MockLayer>(SkPath());

  EXPECT_EQ(mock_layer->parent_has_platform_view(), false);
  mock_layer->set_parent_has_platform_view(true);
  EXPECT_EQ(mock_layer->parent_has_platform_view(), true);

  EXPECT_EQ(mock_layer->parent_has_drawable_image_layer(), false);
  mock_layer->set_parent_has_drawable_image_layer(true);
  EXPECT_EQ(mock_layer->parent_has_drawable_image_layer(), true);

  EXPECT_EQ(mock_layer->parent_has_punch_hole_layer(), false);
  mock_layer->set_parent_has_punch_hole_layer(true);
  EXPECT_EQ(mock_layer->parent_has_punch_hole_layer(), true);

  EXPECT_EQ(mock_layer->fake_has_platform_view(), false);
  mock_layer->set_fake_has_platform_view(true);
  EXPECT_EQ(mock_layer->fake_has_platform_view(), true);

  EXPECT_EQ(mock_layer->fake_reads_surface(), false);
  mock_layer->set_fake_reads_surface(true);
  EXPECT_EQ(mock_layer->fake_reads_surface(), true);

  EXPECT_EQ(mock_layer->fake_opacity_compatible(), false);
  mock_layer->set_fake_opacity_compatible(true);
  EXPECT_EQ(mock_layer->fake_opacity_compatible(), true);

  EXPECT_EQ(mock_layer->fake_has_drawable_image_layer(), false);
  mock_layer->set_fake_has_drawable_image_layer(true);
  EXPECT_EQ(mock_layer->fake_has_drawable_image_layer(), true);

  EXPECT_EQ(mock_layer->fake_has_punch_hole_layer(), false);
  mock_layer->set_fake_has_punch_hole_layer(true);
  EXPECT_EQ(mock_layer->fake_has_punch_hole_layer(), true);
}

}  // namespace testing
}  // namespace clay
