// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <stddef.h>

#include "base/include/fml/macros.h"
#include "clay/flow/compositor_context.h"
#include "clay/flow/layers/clip_rect_layer.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/canvas_test.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {
class LayerTreeTest : public CanvasTest {
 public:
  LayerTreeTest()
      : layer_tree_({64, 64}, 1.0f),
        root_transform_(skity::Matrix::Translate(1.0f, 1.0f)),
        scoped_frame_(compositor_context_.AcquireFrame(
            nullptr, &mock_canvas(), nullptr, root_transform_, false, true)) {}

  LayerTree& layer_tree() { return layer_tree_; }
  CompositorContext::ScopedFrame& frame() { return *scoped_frame_.get(); }
  const skity::Matrix& root_transform() { return root_transform_; }

 private:
  LayerTree layer_tree_;
  CompositorContext compositor_context_;
  skity::Matrix root_transform_;
  std::unique_ptr<CompositorContext::ScopedFrame> scoped_frame_;
};

TEST_F(LayerTreeTest, PaintingEmptyLayerDies) {
  auto layer = std::make_shared<ContainerLayer>();

  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(layer->paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_TRUE(layer->is_empty());

  layer_tree().Paint(frame());
}

TEST_F(LayerTreeTest, PaintBeforePrerollDies) {
  const SkRect child_bounds = SkRect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  SkPath child_path;
  child_path.addRect(child_bounds);
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer);

  layer_tree().set_root_layer(layer);
  EXPECT_EQ(mock_layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_TRUE(mock_layer->is_empty());
  EXPECT_TRUE(layer->is_empty());

  layer_tree().Paint(frame());
  EXPECT_EQ(mock_canvas().draw_calls(), std::vector<MockCanvas::DrawCall>());
}

TEST_F(LayerTreeTest, BeginRetainedPrerollWithoutRootLayerIsNoOp) {
  EXPECT_EQ(layer_tree().retained_preroll_generation(), 0u);

  layer_tree().BeginRetainedPreroll();

  EXPECT_EQ(layer_tree().retained_preroll_generation(), 0u);
}

TEST_F(LayerTreeTest, Simple) {
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kCyan);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer);

  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), mock_layer->paint_bounds());
  EXPECT_FALSE(mock_layer->is_empty());
  EXPECT_FALSE(layer->is_empty());
  EXPECT_EQ(mock_layer->parent_matrix(), root_transform());

  layer_tree().Paint(frame());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({MockCanvas::DrawCall{
                0, MockCanvas::DrawPathData{child_path, child_paint}}}));
}

TEST_F(LayerTreeTest, RasterQuickRejectsSubtreeOutsideFrame) {
  const skity::Rect clip_bounds =
      skity::Rect::MakeXYWH(100.0f, 100.0f, 10.0f, 10.0f);
  const SkPath child_path =
      SkPath().addRect(clip_bounds.Left(), clip_bounds.Top(),
                       clip_bounds.Right(), clip_bounds.Bottom());
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto clip_layer =
      std::make_shared<ClipRectLayer>(clip_bounds, Clip::hardEdge);
  clip_layer->Add(mock_layer);

  layer_tree().set_root_layer(clip_layer);
  frame().Raster(layer_tree(), true, nullptr);

  EXPECT_EQ(mock_layer->paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_EQ(clip_layer->paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_EQ(clip_layer->child_paint_bounds(), skity::Rect::MakeEmpty());
}

TEST_F(LayerTreeTest,
       RetainedPrerollRebuildsQuickRejectedSubtreeWhenVisibleAgain) {
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(40.0f, 40.0f, 50.0f, 50.0f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  auto child = std::make_shared<MockCacheableLayer>(child_path, SkPaint(), 1);
  auto retained_clip = std::make_shared<ClipRectLayer>(
      skity::Rect::MakeLTRB(32.0f, 32.0f, 56.0f, 56.0f), Clip::hardEdge);
  retained_clip->Add(child);

  LayerTree visible_tree({64, 64}, 1.0f);
  auto visible_root = std::make_shared<ContainerLayer>();
  visible_root->Add(retained_clip);
  visible_tree.set_root_layer(visible_root);

  auto& raster_cache = frame().context().raster_cache();
  const RasterCacheKeyID child_cache_key(child->unique_id(),
                                         RasterCacheKeyType::kLayer);

  raster_cache.BeginFrame();
  FrameDamage initial_damage;
  EXPECT_EQ(frame().Raster(visible_tree, false, &initial_damage),
            RasterStatus::kSuccess);
  ASSERT_EQ(child->preroll_count(), 1u);
  ASSERT_EQ(retained_clip->paint_bounds(), child_bounds);
  EXPECT_EQ(raster_cache.GetAccessCount(child_cache_key, root_transform()), 1);
  raster_cache.EndFrame();

  LayerTree rejected_tree({64, 64}, 1.0f);
  auto rejected_root = std::make_shared<ContainerLayer>();
  rejected_root->Add(retained_clip);
  rejected_tree.set_root_layer(rejected_root);

  raster_cache.BeginFrame();
  FrameDamage outside_damage;
  outside_damage.SetPreviousLayerTree(&visible_tree);
  outside_damage.AddAdditionalDamage(
      skity::Rect::MakeLTRB(8.0f, 8.0f, 12.0f, 12.0f));
  EXPECT_EQ(frame().Raster(rejected_tree, false, &outside_damage),
            RasterStatus::kSuccess);
  ASSERT_EQ(child->preroll_count(), 1u);
  ASSERT_EQ(retained_clip->paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_EQ(raster_cache.GetAccessCount(child_cache_key, root_transform()), -1);
  raster_cache.EndFrame();

  LayerTree visible_again_tree({64, 64}, 1.0f);
  auto visible_again_root = std::make_shared<ContainerLayer>();
  visible_again_root->Add(retained_clip);
  visible_again_tree.set_root_layer(visible_again_root);

  raster_cache.BeginFrame();
  FrameDamage visible_damage;
  visible_damage.SetPreviousLayerTree(&rejected_tree);
  visible_damage.AddAdditionalDamage(child_bounds);
  EXPECT_EQ(frame().Raster(visible_again_tree, false, &visible_damage),
            RasterStatus::kSuccess);
  EXPECT_EQ(child->preroll_count(), 2u);
  EXPECT_EQ(child->paint_bounds(), child_bounds);
  EXPECT_EQ(retained_clip->paint_bounds(), child_bounds);
  EXPECT_EQ(raster_cache.GetAccessCount(child_cache_key, root_transform()), 1);
  raster_cache.EndFrame();
}

TEST_F(LayerTreeTest, RetainedPrerollSkipsChildrenOutsideDamage) {
  const SkPath damaged_path = SkPath().addRect(5.0f, 5.0f, 20.0f, 20.0f);
  const SkPath retained_path = SkPath().addRect(40.0f, 40.0f, 50.0f, 50.0f);
  auto damaged_child = std::make_shared<MockLayer>(damaged_path);
  auto retained_child = std::make_shared<MockLayer>(retained_path);
  auto retained_container = std::make_shared<ContainerLayer>();
  retained_container->Add(damaged_child);
  retained_container->Add(retained_child);

  LayerTree previous_tree({64, 64}, 1.0f);
  auto previous_root = std::make_shared<ContainerLayer>();
  previous_root->Add(retained_container);
  previous_tree.set_root_layer(previous_root);

  FrameDamage initial_damage;
  EXPECT_EQ(frame().Raster(previous_tree, false, &initial_damage),
            RasterStatus::kSuccess);
  ASSERT_EQ(damaged_child->preroll_count(), 1u);
  ASSERT_EQ(retained_child->preroll_count(), 1u);

  LayerTree current_tree({64, 64}, 1.0f);
  auto current_root = std::make_shared<ContainerLayer>();
  current_root->Add(retained_container);
  current_tree.set_root_layer(current_root);

  FrameDamage partial_damage;
  partial_damage.SetPreviousLayerTree(&previous_tree);
  partial_damage.AddAdditionalDamage(
      skity::Rect::MakeLTRB(8.0f, 8.0f, 12.0f, 12.0f));
  EXPECT_EQ(frame().Raster(current_tree, false, &partial_damage),
            RasterStatus::kSuccess);

  EXPECT_EQ(damaged_child->preroll_count(), 2u);
  EXPECT_EQ(retained_child->preroll_count(), 1u);
  EXPECT_EQ(retained_container->paint_bounds(),
            skity::Rect::MakeLTRB(5.0f, 5.0f, 50.0f, 50.0f));
}

TEST_F(LayerTreeTest, RetainedPrerollDoesNotSkipPlatformViewSubtree) {
  const SkPath child_path = SkPath().addRect(40.0f, 40.0f, 50.0f, 50.0f);
  auto child = std::make_shared<MockLayer>(child_path);
  child->set_fake_has_platform_view(true);

  LayerTree previous_tree({64, 64}, 1.0f);
  auto previous_root = std::make_shared<ContainerLayer>();
  previous_root->Add(child);
  previous_tree.set_root_layer(previous_root);

  FrameDamage initial_damage;
  EXPECT_EQ(frame().Raster(previous_tree, false, &initial_damage),
            RasterStatus::kSuccess);
  ASSERT_EQ(child->preroll_count(), 1u);

  LayerTree current_tree({64, 64}, 1.0f);
  auto current_root = std::make_shared<ContainerLayer>();
  current_root->Add(child);
  current_tree.set_root_layer(current_root);

  FrameDamage partial_damage;
  partial_damage.SetPreviousLayerTree(&previous_tree);
  partial_damage.AddAdditionalDamage(
      skity::Rect::MakeLTRB(8.0f, 8.0f, 12.0f, 12.0f));
  EXPECT_EQ(frame().Raster(current_tree, false, &partial_damage),
            RasterStatus::kSuccess);

  EXPECT_EQ(child->preroll_count(), 2u);
}

TEST_F(LayerTreeTest, Multiple) {
  const SkPath child_path1 = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path2 = SkPath().addRect(8.0f, 2.0f, 16.5f, 14.5f);
  const SkPaint child_paint1(SkColors::kGray);
  const SkPaint child_paint2(SkColors::kGreen);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  mock_layer1->set_fake_has_platform_view(true);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  skity::Rect expected_total_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  expected_total_bounds.Join(
      clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_total_bounds);
  EXPECT_FALSE(mock_layer1->is_empty());
  EXPECT_FALSE(mock_layer2->is_empty());
  EXPECT_FALSE(layer->is_empty());
  EXPECT_EQ(mock_layer1->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer2->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(),
            kGiantRect);  // Siblings are independent

  layer_tree().Paint(frame());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0, MockCanvas::DrawPathData{child_path1, child_paint1}},
                   MockCanvas::DrawCall{0, MockCanvas::DrawPathData{
                                               child_path2, child_paint2}}}));
}

TEST_F(LayerTreeTest, MultipleWithEmpty) {
  const SkPath child_path1 = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPaint child_paint1(SkColors::kGray);
  const SkPaint child_paint2(SkColors::kGreen);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(SkPath(), child_paint2);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(SkPath().getBounds()));
  EXPECT_EQ(layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_FALSE(mock_layer1->is_empty());
  EXPECT_TRUE(mock_layer2->is_empty());
  EXPECT_FALSE(layer->is_empty());
  EXPECT_EQ(mock_layer1->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer2->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(), kGiantRect);

  layer_tree().Paint(frame());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({MockCanvas::DrawCall{
                0, MockCanvas::DrawPathData{child_path1, child_paint1}}}));
}

TEST_F(LayerTreeTest, NeedsSystemComposite) {
  const SkPath child_path1 = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path2 = SkPath().addRect(8.0f, 2.0f, 16.5f, 14.5f);
  const SkPaint child_paint1(SkColors::kGray);
  const SkPaint child_paint2(SkColors::kGreen);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  skity::Rect expected_total_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  expected_total_bounds.Join(
      clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_total_bounds);
  EXPECT_FALSE(mock_layer1->is_empty());
  EXPECT_FALSE(mock_layer2->is_empty());
  EXPECT_FALSE(layer->is_empty());
  EXPECT_EQ(mock_layer1->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer2->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(), kGiantRect);

  layer_tree().Paint(frame());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0, MockCanvas::DrawPathData{child_path1, child_paint1}},
                   MockCanvas::DrawCall{0, MockCanvas::DrawPathData{
                                               child_path2, child_paint2}}}));
}

TEST_F(LayerTreeTest, PrerollContextInitialization) {
  LayerStateStack state_stack;
  state_stack.set_preroll_delegate(kGiantRect, skity::Matrix());
  FixedRefreshRateStopwatch mock_raster_time;
  FixedRefreshRateStopwatch mock_ui_time;
  std::shared_ptr<DrawableImageRegistry> mock_registry;

  auto expect_defaults = [&state_stack, &mock_raster_time, &mock_ui_time,
                          &mock_registry](const PrerollContext& context) {
    EXPECT_EQ(context.raster_cache, nullptr);
    EXPECT_EQ(context.gr_context, nullptr);
    EXPECT_EQ(context.compositor_state, nullptr);
    EXPECT_EQ(&context.state_stack, &state_stack);
    EXPECT_EQ(context.dst_color_space, nullptr);
    EXPECT_EQ(context.state_stack.device_cull_rect(), kGiantRect);
    EXPECT_EQ(context.state_stack.transform_4x4(), skity::Matrix());
    EXPECT_EQ(context.surface_needs_readback, false);

    EXPECT_EQ(&context.raster_time, &mock_raster_time);
    EXPECT_EQ(&context.ui_time, &mock_ui_time);
    EXPECT_EQ(context.drawable_image_registry.get(), mock_registry.get());
    EXPECT_EQ(context.frame_device_pixel_ratio, 1.0f);

    EXPECT_EQ(context.has_platform_view, false);
    EXPECT_EQ(context.has_drawable_image_layer, false);
    EXPECT_EQ(context.has_punch_hole_layer, false);
    EXPECT_EQ(context.subtree_was_quick_rejected, false);

    EXPECT_EQ(context.renderable_state_flags, 0);
    EXPECT_EQ(context.raster_cached_entries, nullptr);
  };

  // These 4 initializers are required because they are handled by reference
  PrerollContext context{
      .state_stack = state_stack,
      .raster_time = mock_raster_time,
      .ui_time = mock_ui_time,
      .drawable_image_registry = mock_registry,
  };
  expect_defaults(context);
}

TEST_F(LayerTreeTest, PaintContextInitialization) {
  LayerStateStack state_stack;
  FixedRefreshRateStopwatch mock_raster_time;
  FixedRefreshRateStopwatch mock_ui_time;
  std::shared_ptr<DrawableImageRegistry> mock_registry;

  auto expect_defaults = [&state_stack, &mock_raster_time, &mock_ui_time,
                          &mock_registry](const PaintContext& context) {
    EXPECT_EQ(&context.state_stack, &state_stack);
    EXPECT_EQ(context.canvas, nullptr);
    EXPECT_EQ(context.gr_context, nullptr);
    EXPECT_EQ(context.compositor_state, nullptr);
    EXPECT_EQ(&context.raster_time, &mock_raster_time);
    EXPECT_EQ(&context.ui_time, &mock_ui_time);
    EXPECT_EQ(context.drawable_image_registry.get(), mock_registry.get());
    EXPECT_EQ(context.raster_cache, nullptr);
    EXPECT_EQ(context.state_stack.checkerboard_func(), nullptr);
    EXPECT_EQ(context.frame_device_pixel_ratio, 1.0f);
  };

  // These 4 initializers are required because they are handled by reference
  PaintContext context{
      .state_stack = state_stack,
      .raster_time = mock_raster_time,
      .ui_time = mock_ui_time,
      .drawable_image_registry = mock_registry,
  };
  expect_defaults(context);
}

}  // namespace testing
}  // namespace clay
