// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/compositor/compositor_state.h"
#include "clay/flow/layers/clip_rect_layer.h"
#include "clay/flow/layers/platform_view_layer.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/mock_canvas.h"

namespace clay {
namespace testing {

using PlatformViewLayerTest = LayerTest;

TEST_F(PlatformViewLayerTest, NullViewEmbedderDoesntPrerollCompositeOrPaint) {
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  const int64_t view_id = 0;
  auto layer =
      std::make_shared<PlatformViewLayer>(layer_offset, layer_size, view_id);

  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->has_platform_view);
  EXPECT_EQ(layer->paint_bounds(),
            skity::Rect::MakeSize({layer_size.x, layer_size.y})
                .MakeOffset(layer_offset.x, layer_offset.y));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_FALSE(layer->subtree_has_platform_view());

  layer->Paint(paint_context());
  EXPECT_EQ(paint_context().canvas, &mock_canvas());
  EXPECT_EQ(mock_canvas().draw_calls(), std::vector<MockCanvas::DrawCall>());
}

TEST_F(PlatformViewLayerTest, ClippedPlatformViewDoesNotComposite) {
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  const skity::Rect child_clip =
      skity::Rect::MakeLTRB(20.0f, 20.0f, 40.0f, 40.0f);
  const skity::Rect parent_clip =
      skity::Rect::MakeLTRB(50.0f, 50.0f, 80.0f, 80.0f);
  const int64_t view_id = 0;
  auto layer =
      std::make_shared<PlatformViewLayer>(layer_offset, layer_size, view_id);
  auto child_clip_layer =
      std::make_shared<ClipRectLayer>(child_clip, Clip::hardEdge);
  auto parent_clip_layer =
      std::make_shared<ClipRectLayer>(parent_clip, Clip::hardEdge);
  parent_clip_layer->Add(child_clip_layer);
  child_clip_layer->Add(layer);

  CompositorState compositor_state({64, 64});
  preroll_context()->compositor_state = &compositor_state;

  parent_clip_layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->has_platform_view);
  EXPECT_TRUE(compositor_state.GetCompositionOrder().empty());
  EXPECT_EQ(layer->paint_bounds(),
            skity::Rect::MakeSize({layer_size.x, layer_size.y})
                .MakeOffset(layer_offset.x, layer_offset.y));
  EXPECT_FALSE(layer->subtree_has_platform_view());
  EXPECT_FALSE(child_clip_layer->subtree_has_platform_view());
  EXPECT_FALSE(parent_clip_layer->subtree_has_platform_view());
  EXPECT_FALSE(parent_clip_layer->needs_painting(paint_context()));
}

TEST_F(PlatformViewLayerTest, OffscreenPlatformViewDoesNotComposite) {
  const skity::Vec2 layer_offset = skity::Vec2(80.0f, 80.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  const int64_t view_id = 0;
  auto layer =
      std::make_shared<PlatformViewLayer>(layer_offset, layer_size, view_id);

  CompositorState compositor_state({64, 64});
  preroll_context()->compositor_state = &compositor_state;
  preroll_context()->state_stack.set_preroll_delegate(
      skity::Rect::MakeLTRB(0.0f, 0.0f, 64.0f, 64.0f));

  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->has_platform_view);
  EXPECT_TRUE(compositor_state.GetCompositionOrder().empty());
  EXPECT_FALSE(layer->subtree_has_platform_view());

  layer->Paint(paint_context());
  EXPECT_EQ(paint_context().canvas, &mock_canvas());
}

TEST_F(PlatformViewLayerTest, CulledPlatformViewCanCompositeAgain) {
  const skity::Vec2 layer_offset = skity::Vec2(80.0f, 80.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  const int64_t view_id = 0;
  auto layer =
      std::make_shared<PlatformViewLayer>(layer_offset, layer_size, view_id);

  CompositorState culled_compositor_state({64, 64});
  preroll_context()->compositor_state = &culled_compositor_state;
  preroll_context()->state_stack.set_preroll_delegate(
      skity::Rect::MakeLTRB(0.0f, 0.0f, 64.0f, 64.0f));

  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->has_platform_view);
  EXPECT_TRUE(culled_compositor_state.GetCompositionOrder().empty());
  EXPECT_FALSE(layer->subtree_has_platform_view());

  CompositorState visible_compositor_state({128, 128});
  preroll_context()->compositor_state = &visible_compositor_state;
  preroll_context()->state_stack.set_preroll_delegate(
      skity::Rect::MakeLTRB(0.0f, 0.0f, 128.0f, 128.0f));

  layer->Preroll(preroll_context());
  EXPECT_TRUE(preroll_context()->has_platform_view);
  EXPECT_EQ(visible_compositor_state.GetCompositionOrder(),
            std::vector<int64_t>({view_id}));
  EXPECT_TRUE(layer->subtree_has_platform_view());

  paint_context().compositor_state = &visible_compositor_state;
  layer->Paint(paint_context());
  EXPECT_EQ(paint_context().canvas,
            visible_compositor_state.GetSlices()[view_id]->canvas());
}

TEST_F(PlatformViewLayerTest, OpacityInheritance) {
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  const int64_t view_id = 0;
  auto layer =
      std::make_shared<PlatformViewLayer>(layer_offset, layer_size, view_id);

  PrerollContext* context = preroll_context();
  layer->Preroll(preroll_context());
  EXPECT_EQ(context->renderable_state_flags, 0);
}

TEST_F(PlatformViewLayerTest, StateTransfer) {
  const skity::Matrix transform1 = skity::Matrix::Translate(5, 5);
  const skity::Matrix transform2 = skity::Matrix::Translate(15, 15);
  const skity::Matrix combined_transform = skity::Matrix::Translate(20, 20);
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  const int64_t view_id = 0;
  const SkPath path1 = SkPath().addOval({10, 10, 20, 20});
  const SkPath path2 = SkPath().addOval({15, 15, 30, 30});

  // transform_layer1
  //   |- child1
  //   |- platform_layer
  //   |- transform_layer2
  //        |- child2
  auto transform_layer1 = std::make_shared<TransformLayer>(transform1);
  auto transform_layer2 = std::make_shared<TransformLayer>(transform2);
  auto platform_layer =
      std::make_shared<PlatformViewLayer>(layer_offset, layer_size, view_id);
  auto child1 = std::make_shared<MockLayer>(path1);
  child1->set_expected_paint_matrix(transform1);
  auto child2 = std::make_shared<MockLayer>(path2);
  child2->set_expected_paint_matrix(combined_transform);
  transform_layer1->Add(child1);
  transform_layer1->Add(platform_layer);
  transform_layer1->Add(transform_layer2);
  transform_layer2->Add(child2);

  CompositorState compositor_state({64, 64});

  PrerollContext* preroll_ctx = preroll_context();
  preroll_ctx->compositor_state = &compositor_state;
  transform_layer1->Preroll(preroll_ctx);

  PaintContext& paint_ctx = paint_context();
  transform_layer1->Paint(paint_ctx);
}

}  // namespace testing
}  // namespace clay
