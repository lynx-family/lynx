// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/testing/diff_context_test.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace clay {
namespace testing {

using ContainerLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(ContainerLayerTest, LayerWithParentHasPlatformView) {
  auto layer = std::make_shared<ContainerLayer>();

  preroll_context()->has_platform_view = true;
  EXPECT_DEATH_IF_SUPPORTED(layer->Preroll(preroll_context()),
                            "!context->has_platform_view");
}

TEST_F(ContainerLayerTest, LayerWithParentHasDrawableImageLayer) {
  auto layer = std::make_shared<ContainerLayer>();

  preroll_context()->has_drawable_image_layer = true;
  EXPECT_DEATH_IF_SUPPORTED(layer->Preroll(preroll_context()),
                            "!context->has_drawable_image_layer");
}

TEST_F(ContainerLayerTest, LayerWithParentHasPunchHoleLayer) {
  auto layer = std::make_shared<ContainerLayer>();
  preroll_context()->has_punch_hole_layer = true;
  EXPECT_DEATH_IF_SUPPORTED(layer->Preroll(preroll_context()),
                            "!context->has_punch_hole_layer");
}

TEST_F(ContainerLayerTest, PaintingEmptyLayerDies) {
  auto layer = std::make_shared<ContainerLayer>();

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_EQ(layer->child_paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(ContainerLayerTest, PaintBeforePrerollDies) {
  SkPath child_path;
  child_path.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer);

  EXPECT_EQ(layer->paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_EQ(layer->child_paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(ContainerLayerTest, LayerWithParentHasDrawableImageLayerNeedsResetFlag) {
  SkPath child_path1;
  child_path1.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  SkPath child_path2;
  child_path2.addRect(8.0f, 2.0f, 16.5f, 14.5f);
  SkPaint child_paint1(SkColors::kGray);
  SkPaint child_paint2(SkColors::kGreen);

  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  mock_layer1->set_fake_has_drawable_image_layer(true);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);

  auto root = std::make_shared<ContainerLayer>();
  auto container_layer1 = std::make_shared<ContainerLayer>();
  auto container_layer2 = std::make_shared<ContainerLayer>();
  root->Add(container_layer1);
  root->Add(container_layer2);
  container_layer1->Add(mock_layer1);
  container_layer2->Add(mock_layer2);

  EXPECT_EQ(preroll_context()->has_drawable_image_layer, false);
  root->Preroll(preroll_context());
  EXPECT_EQ(preroll_context()->has_drawable_image_layer, true);
  // The flag for holding drawable image layer from parent needs to be clear
  EXPECT_EQ(mock_layer2->parent_has_drawable_image_layer(), false);
}

TEST_F(ContainerLayerTest, LayerWithParentHasPunchHoleLayerNeedsResetFlag) {
  SkPath child_path1;
  child_path1.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  SkPath child_path2;
  child_path2.addRect(8.0f, 2.0f, 16.5f, 14.5f);
  SkPaint child_paint1(SkColors::kGray);
  SkPaint child_paint2(SkColors::kGreen);

  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  mock_layer1->set_fake_has_punch_hole_layer(true);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);

  auto root = std::make_shared<ContainerLayer>();
  auto container_layer1 = std::make_shared<ContainerLayer>();
  auto container_layer2 = std::make_shared<ContainerLayer>();
  root->Add(container_layer1);
  root->Add(container_layer2);
  container_layer1->Add(mock_layer1);
  container_layer2->Add(mock_layer2);

  EXPECT_EQ(preroll_context()->has_punch_hole_layer, false);
  root->Preroll(preroll_context());
  EXPECT_EQ(preroll_context()->has_punch_hole_layer, true);
  // The flag for holding punch hole layer from parent needs to be clear
  EXPECT_EQ(mock_layer2->parent_has_punch_hole_layer(), false);
}

TEST_F(ContainerLayerTest, Simple) {
  SkPath child_path;
  child_path.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  SkPaint child_paint(SkColors::kGreen);
  skity::Matrix initial_transform = skity::Matrix::Translate(-0.5f, -0.5f);

  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->has_platform_view);
  EXPECT_EQ(mock_layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  EXPECT_EQ(layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  EXPECT_EQ(layer->child_paint_bounds(), layer->paint_bounds());
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer->parent_cull_rect(), kGiantRect);

  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({MockCanvas::DrawCall{
                0, MockCanvas::DrawPathData{child_path, child_paint}}}));
}

TEST_F(ContainerLayerTest, Multiple) {
  SkPath child_path1;
  child_path1.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  SkPath child_path2;
  child_path2.addRect(8.0f, 2.0f, 16.5f, 14.5f);
  SkPaint child_paint1(SkColors::kGray);
  SkPaint child_paint2(SkColors::kGreen);
  skity::Matrix initial_transform = skity::Matrix::Translate(-0.5f, -0.5f);

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
  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_TRUE(preroll_context()->has_platform_view);
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_total_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), layer->paint_bounds());
  EXPECT_TRUE(mock_layer1->needs_painting(paint_context()));
  EXPECT_TRUE(mock_layer2->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer1->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer2->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(),
            kGiantRect);  // Siblings are independent

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0, MockCanvas::DrawPathData{child_path1, child_paint1}},
                   MockCanvas::DrawCall{0, MockCanvas::DrawPathData{
                                               child_path2, child_paint2}}}));
}

TEST_F(ContainerLayerTest, MultipleWithEmpty) {
  SkPath child_path1;
  child_path1.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  SkPaint child_paint1(SkColors::kGray);
  SkPaint child_paint2(SkColors::kGreen);
  skity::Matrix initial_transform = skity::Matrix::Translate(-0.5f, -0.5f);

  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(SkPath(), child_paint2);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->has_platform_view);
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(SkPath().getBounds()));
  EXPECT_EQ(layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(layer->child_paint_bounds(), layer->paint_bounds());
  EXPECT_TRUE(mock_layer1->needs_painting(paint_context()));
  EXPECT_FALSE(mock_layer2->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer1->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer2->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(), kGiantRect);

  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({MockCanvas::DrawCall{
                0, MockCanvas::DrawPathData{child_path1, child_paint1}}}));
}

TEST_F(ContainerLayerTest, NeedsSystemComposite) {
  SkPath child_path1;
  child_path1.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  SkPath child_path2;
  child_path2.addRect(8.0f, 2.0f, 16.5f, 14.5f);
  SkPaint child_paint1(SkColors::kGray);
  SkPaint child_paint2(SkColors::kGreen);
  skity::Matrix initial_transform = skity::Matrix::Translate(-0.5f, -0.5f);

  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  mock_layer1->set_fake_has_platform_view(false);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  skity::Rect expected_total_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  expected_total_bounds.Join(
      clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->has_platform_view);
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_total_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), layer->paint_bounds());
  EXPECT_TRUE(mock_layer1->needs_painting(paint_context()));
  EXPECT_TRUE(mock_layer2->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer1->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer2->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(), kGiantRect);

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0, MockCanvas::DrawPathData{child_path1, child_paint1}},
                   MockCanvas::DrawCall{0, MockCanvas::DrawPathData{
                                               child_path2, child_paint2}}}));
}

TEST_F(ContainerLayerTest, RasterCacheTest) {
  // LTRB
  const SkPath child_path1 = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path2 = SkPath().addRect(21.0f, 6.0f, 25.5f, 21.5f);
  const SkPath child_path3 = SkPath().addRect(26.0f, 6.0f, 30.5f, 21.5f);
  const SkPaint child_paint1(SkColors::kGray);
  const SkPaint child_paint2(SkColors::kGreen);
  const SkPaint paint;
  auto cacheable_container_layer1 =
      MockCacheableContainerLayer::CacheLayerOrChildren();
  auto cacheable_container_layer2 =
      MockCacheableContainerLayer::CacheLayerOnly();
  auto cacheable_container_layer11 =
      MockCacheableContainerLayer::CacheLayerOrChildren();

  auto cacheable_layer111 =
      std::make_shared<MockCacheableLayer>(child_path3, paint);
  // if the frame had rendered 2 frames, we will cache the cacheable_layer21
  // layer
  auto cacheable_layer21 =
      std::make_shared<MockCacheableLayer>(child_path1, paint, 2);
  //                                     layer
  //                                       |
  //                 ______________________ ______________________
  //                 |                     |                      |
  //  cacheable_container_layer1      mock_layer2     cacheable_container_layer2
  //                 |                                            |
  // cacheable_container_layer11                         cacheable_layer21
  //                 |
  //        cacheable_layer111

  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(SkPath(), child_paint2);
  auto mock_layer3 = std::make_shared<MockLayer>(child_path2, paint);

  cacheable_container_layer1->Add(mock_layer1);
  cacheable_container_layer1->Add(mock_layer3);

  cacheable_container_layer1->Add(cacheable_container_layer11);
  cacheable_container_layer11->Add(cacheable_layer111);

  cacheable_container_layer2->Add(cacheable_layer21);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(cacheable_container_layer1);
  layer->Add(mock_layer2);
  layer->Add(cacheable_container_layer2);

  SkCanvas cache_canvas;
  cache_canvas.setMatrix(SkMatrix::I());

  // Initial Preroll for check the layer paint bounds
  layer->Preroll(preroll_context());

  EXPECT_EQ(mock_layer1->paint_bounds(),
            skity::Rect::MakeLTRB(5.f, 6.f, 20.5f, 21.5f));
  EXPECT_EQ(mock_layer3->paint_bounds(),
            skity::Rect::MakeLTRB(21.0f, 6.0f, 25.5f, 21.5f));
  EXPECT_EQ(cacheable_layer111->paint_bounds(),
            skity::Rect::MakeLTRB(26.0f, 6.0f, 30.5f, 21.5f));
  EXPECT_EQ(cacheable_container_layer1->paint_bounds(),
            skity::Rect::MakeLTRB(5.f, 6.f, 30.5f, 21.5f));

  // the preroll context's raster cache is nullptr
  EXPECT_EQ(preroll_context()->raster_cached_entries->size(),
            static_cast<size_t>(0));
  {
    // frame1
    use_mock_raster_cache();
    preroll_context()->raster_cache->BeginFrame();
    layer->Preroll(preroll_context());
    preroll_context()->raster_cache->EvictUnusedCacheEntries();
    // Cache the cacheable entries
    LayerTree::TryToRasterCache(*(preroll_context()->raster_cached_entries),
                                &paint_context());

    EXPECT_EQ(preroll_context()->raster_cached_entries->size(),
              static_cast<size_t>(5));

    // cacheable_container_layer1 will cache his children
    EXPECT_EQ(cacheable_container_layer1->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kChildren);
    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_container_layer1->raster_cache_item()->GetId().value(),
        skity::Matrix()));

    EXPECT_EQ(cacheable_container_layer11->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kChildren);
    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_container_layer11->raster_cache_item()->GetId().value(),
        skity::Matrix()));
    EXPECT_FALSE(raster_cache()->Draw(
        cacheable_container_layer11->raster_cache_item()->GetId().value(),
        cache_canvas, &paint));

    // The cacheable_layer111 should be cached when rended 3 frames
    EXPECT_EQ(cacheable_layer111->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kNone);
    // render count < 2 don't cache it
    EXPECT_EQ(cacheable_container_layer2->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kNone);
    preroll_context()->raster_cache->EndFrame();
  }

  {
    // frame2
    // new frame the layer tree will create new PrerollContext, so in here we
    // clear the cached_entries
    preroll_context()->raster_cached_entries->clear();
    preroll_context()->raster_cache->BeginFrame();
    layer->Preroll(preroll_context());
    preroll_context()->raster_cache->EvictUnusedCacheEntries();

    // Cache the cacheable entries
    LayerTree::TryToRasterCache(*(preroll_context()->raster_cached_entries),
                                &paint_context());
    EXPECT_EQ(preroll_context()->raster_cached_entries->size(),
              static_cast<size_t>(5));
    EXPECT_EQ(cacheable_container_layer1->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kChildren);
    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_container_layer1->raster_cache_item()->GetId().value(),
        skity::Matrix()));

    EXPECT_EQ(cacheable_container_layer11->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kChildren);
    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_container_layer11->raster_cache_item()->GetId().value(),
        skity::Matrix()));
    EXPECT_FALSE(raster_cache()->Draw(
        cacheable_container_layer11->raster_cache_item()->GetId().value(),
        cache_canvas, &paint));

    EXPECT_EQ(cacheable_container_layer2->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kNone);

    // render count == 2 cache it
    EXPECT_EQ(cacheable_layer21->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kCurrent);
    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_layer21->raster_cache_item()->GetId().value(),
        skity::Matrix()));
    EXPECT_TRUE(raster_cache()->Draw(
        cacheable_layer21->raster_cache_item()->GetId().value(), cache_canvas,
        &paint));
    preroll_context()->raster_cache->EndFrame();
  }

  {
    // frame3
    // new frame the layer tree will create new PrerollContext, so in here we
    // clear the cached_entries
    preroll_context()->raster_cache->BeginFrame();
    preroll_context()->raster_cached_entries->clear();
    layer->Preroll(preroll_context());
    preroll_context()->raster_cache->EvictUnusedCacheEntries();
    // Cache the cacheable entries
    LayerTree::TryToRasterCache(*(preroll_context()->raster_cached_entries),
                                &paint_context());
    EXPECT_EQ(preroll_context()->raster_cached_entries->size(),
              static_cast<size_t>(5));
    EXPECT_EQ(cacheable_container_layer1->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kCurrent);
    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_container_layer1->raster_cache_item()->GetId().value(),
        skity::Matrix()));
    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_container_layer11->raster_cache_item()->GetId().value(),
        skity::Matrix()));
    EXPECT_FALSE(raster_cache()->Draw(
        cacheable_container_layer11->raster_cache_item()->GetId().value(),
        cache_canvas, &paint));
    // The 3td frame, we will cache the cacheable_layer111, but his ancestor has
    // been cached, so cacheable_layer111 Draw is false
    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_layer111->raster_cache_item()->GetId().value(),
        skity::Matrix()));
    EXPECT_FALSE(raster_cache()->Draw(
        cacheable_layer111->raster_cache_item()->GetId().value(), cache_canvas,
        &paint));

    // The third frame, we will cache the cacheable_container_layer2
    EXPECT_EQ(cacheable_container_layer2->raster_cache_item()->cache_state(),
              RasterCacheItem::CacheState::kCurrent);

    EXPECT_TRUE(raster_cache()->HasEntry(
        cacheable_layer21->raster_cache_item()->GetId().value(),
        skity::Matrix()));
    preroll_context()->raster_cache->EndFrame();
  }

  {
    preroll_context()->raster_cache->BeginFrame();
    // frame4
    preroll_context()->raster_cached_entries->clear();
    layer->Preroll(preroll_context());
    preroll_context()->raster_cache->EvictUnusedCacheEntries();
    LayerTree::TryToRasterCache(*(preroll_context()->raster_cached_entries),
                                &paint_context());
    preroll_context()->raster_cache->EndFrame();

    // frame5
    preroll_context()->raster_cache->BeginFrame();
    preroll_context()->raster_cached_entries->clear();
    layer->Preroll(preroll_context());
    LayerTree::TryToRasterCache(*(preroll_context()->raster_cached_entries),
                                &paint_context());
    preroll_context()->raster_cache->EndFrame();

    // frame6
    preroll_context()->raster_cache->BeginFrame();
    preroll_context()->raster_cached_entries->clear();
    layer->Preroll(preroll_context());
    LayerTree::TryToRasterCache(*(preroll_context()->raster_cached_entries),
                                &paint_context());
    preroll_context()->raster_cache->EndFrame();
  }
}

TEST_F(ContainerLayerTest, OpacityInheritance) {
  auto path1 = SkPath().addRect({10, 10, 30, 30});
  auto mock1 = MockLayer::MakeOpacityCompatible(path1);
  auto container1 = std::make_shared<ContainerLayer>();
  container1->Add(mock1);

  // ContainerLayer will pass through compatibility
  PrerollContext* context = preroll_context();
  container1->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);

  auto path2 = SkPath().addRect({40, 40, 50, 50});
  auto mock2 = MockLayer::MakeOpacityCompatible(path2);
  container1->Add(mock2);

  // ContainerLayer will pass through compatibility from multiple
  // non-overlapping compatible children
  container1->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);

  auto path3 = SkPath().addRect({20, 20, 40, 40});
  auto mock3 = MockLayer::MakeOpacityCompatible(path3);
  container1->Add(mock3);

  // ContainerLayer will not pass through compatibility from multiple
  // overlapping children even if they are individually compatible
  container1->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags, 0);

  auto container2 = std::make_shared<ContainerLayer>();
  container2->Add(mock1);
  container2->Add(mock2);

  // Double check first two children are compatible and non-overlapping
  container2->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);

  auto path4 = SkPath().addRect({60, 60, 70, 70});
  auto mock4 = MockLayer::Make(path4);
  container2->Add(mock4);

  // The third child is non-overlapping, but not compatible so the
  // ContainerLayer should end up incompatible
  container2->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags, 0);
}

TEST_F(ContainerLayerTest, CollectionCacheableLayer) {
  SkPath child_path;
  child_path.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  SkPaint child_paint(SkColors::kGreen);
  skity::Matrix initial_transform = skity::Matrix::Translate(-0.5f, -0.5f);

  auto mock_layer1 = std::make_shared<MockLayer>(SkPath(), child_paint);
  auto mock_cacheable_container_layer1 =
      std::make_shared<MockCacheableContainerLayer>();
  auto mock_container_layer = std::make_shared<ContainerLayer>();
  auto mock_cacheable_layer =
      std::make_shared<MockCacheableLayer>(child_path, child_paint);
  mock_cacheable_container_layer1->Add(mock_cacheable_layer);

  // ContainerLayer
  //   |- MockLayer
  //   |- MockCacheableContainerLayer
  //        |- MockCacheableLayer
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_cacheable_container_layer1);
  layer->Add(mock_layer1);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  // raster cache is null, so no entry
  ASSERT_EQ(preroll_context()->raster_cached_entries->size(),
            static_cast<const size_t>(0));

  use_mock_raster_cache();
  // preroll_context()->raster_cache = raster_cache();
  layer->Preroll(preroll_context());
  ASSERT_EQ(preroll_context()->raster_cached_entries->size(),
            static_cast<const size_t>(2));
}

using ContainerLayerDiffTest = DiffContextTest;

// Insert PictureLayer amongst container layers
TEST_F(ContainerLayerDiffTest, PictureLayerInsertion) {
  auto pic1 = CreatePicture(skity::Rect::MakeLTRB(0, 0, 50, 50), 1);
  auto pic2 = CreatePicture(skity::Rect::MakeLTRB(100, 0, 150, 50), 1);
  auto pic3 = CreatePicture(skity::Rect::MakeLTRB(200, 0, 250, 50), 1);

  MockLayerTree t1;

  auto t1_c1 = CreateContainerLayer(CreatePictureLayer(pic1));
  t1.root()->Add(t1_c1);

  auto t1_c2 = CreateContainerLayer(CreatePictureLayer(pic2));
  t1.root()->Add(t1_c2);

  auto damage = DiffLayerTree(t1, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 150, 50));

  // Add in the middle

  MockLayerTree t2;
  auto t2_c1 = CreateContainerLayer(CreatePictureLayer(pic1));
  t2_c1->AssignOldLayer(t1_c1.get());
  t2.root()->Add(t2_c1);

  t2.root()->Add(CreatePictureLayer(pic3));

  auto t2_c2 = CreateContainerLayer(CreatePictureLayer(pic2));
  t2_c2->AssignOldLayer(t1_c2.get());
  t2.root()->Add(t2_c2);

  damage = DiffLayerTree(t2, t1);
  FML_LOG(ERROR) << "frame_damage: " << damage.frame_damage.Left() << ", "
                 << damage.frame_damage.Top() << ", "
                 << damage.frame_damage.Right() << ", "
                 << damage.frame_damage.Bottom();
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(200, 0, 250, 50));

  // Add in the beginning

  t2 = MockLayerTree();
  t2.root()->Add(CreatePictureLayer(pic3));
  t2.root()->Add(t2_c1);
  t2.root()->Add(t2_c2);
  damage = DiffLayerTree(t2, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(200, 0, 250, 50));

  // Add at the end

  t2 = MockLayerTree();
  t2.root()->Add(t2_c1);
  t2.root()->Add(t2_c2);
  t2.root()->Add(CreatePictureLayer(pic3));
  damage = DiffLayerTree(t2, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(200, 0, 250, 50));
}

// Insert picture layer amongst other picture layers
TEST_F(ContainerLayerDiffTest, PictureInsertion) {
  auto pic1 = CreatePicture(skity::Rect::MakeLTRB(0, 0, 50, 50), 1);
  auto pic2 = CreatePicture(skity::Rect::MakeLTRB(100, 0, 150, 50), 1);
  auto pic3 = CreatePicture(skity::Rect::MakeLTRB(200, 0, 250, 50), 1);

  MockLayerTree t1;
  t1.root()->Add(CreatePictureLayer(pic1));
  t1.root()->Add(CreatePictureLayer(pic2));

  auto damage = DiffLayerTree(t1, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 150, 50));

  MockLayerTree t2;
  t2.root()->Add(CreatePictureLayer(pic3));
  t2.root()->Add(CreatePictureLayer(pic1));
  t2.root()->Add(CreatePictureLayer(pic2));

  damage = DiffLayerTree(t2, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(200, 0, 250, 50));

  MockLayerTree t3;
  t3.root()->Add(CreatePictureLayer(pic1));
  t3.root()->Add(CreatePictureLayer(pic3));
  t3.root()->Add(CreatePictureLayer(pic2));

  damage = DiffLayerTree(t3, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(200, 0, 250, 50));

  MockLayerTree t4;
  t4.root()->Add(CreatePictureLayer(pic1));
  t4.root()->Add(CreatePictureLayer(pic2));
  t4.root()->Add(CreatePictureLayer(pic3));

  damage = DiffLayerTree(t4, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(200, 0, 250, 50));
}

TEST_F(ContainerLayerDiffTest, LayerDeletion) {
  auto path1 = SkPath().addRect(SkRect::MakeLTRB(0, 0, 50, 50));
  auto path2 = SkPath().addRect(SkRect::MakeLTRB(100, 0, 150, 50));
  auto path3 = SkPath().addRect(SkRect::MakeLTRB(200, 0, 250, 50));

  auto c1 = CreateContainerLayer(std::make_shared<MockLayer>(path1));
  auto c2 = CreateContainerLayer(std::make_shared<MockLayer>(path2));
  auto c3 = CreateContainerLayer(std::make_shared<MockLayer>(path3));

  MockLayerTree t1;
  t1.root()->Add(c1);
  t1.root()->Add(c2);
  t1.root()->Add(c3);

  auto damage = DiffLayerTree(t1, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 250, 50));

  MockLayerTree t2;
  t2.root()->Add(c2);
  t2.root()->Add(c3);

  damage = DiffLayerTree(t2, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 50, 50));

  MockLayerTree t3;
  t3.root()->Add(c1);
  t3.root()->Add(c3);

  damage = DiffLayerTree(t3, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(100, 0, 150, 50));

  MockLayerTree t4;
  t4.root()->Add(c1);
  t4.root()->Add(c2);

  damage = DiffLayerTree(t4, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(200, 0, 250, 50));

  MockLayerTree t5;
  t5.root()->Add(c1);

  damage = DiffLayerTree(t5, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(100, 0, 250, 50));

  MockLayerTree t6;
  t6.root()->Add(c2);

  damage = DiffLayerTree(t6, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 250, 50));

  MockLayerTree t7;
  t7.root()->Add(c3);

  damage = DiffLayerTree(t7, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 150, 50));
}

TEST_F(ContainerLayerDiffTest, ReplaceLayer) {
  auto path1 = SkPath().addRect(SkRect::MakeLTRB(0, 0, 50, 50));
  auto path2 = SkPath().addRect(SkRect::MakeLTRB(100, 0, 150, 50));
  auto path3 = SkPath().addRect(SkRect::MakeLTRB(200, 0, 250, 50));

  auto path1a = SkPath().addRect(SkRect::MakeLTRB(0, 100, 50, 150));
  auto path2a = SkPath().addRect(SkRect::MakeLTRB(100, 100, 150, 150));
  auto path3a = SkPath().addRect(SkRect::MakeLTRB(200, 100, 250, 150));

  auto c1 = CreateContainerLayer(std::make_shared<MockLayer>(path1));
  auto c2 = CreateContainerLayer(std::make_shared<MockLayer>(path2));
  auto c3 = CreateContainerLayer(std::make_shared<MockLayer>(path3));

  MockLayerTree t1;
  t1.root()->Add(c1);
  t1.root()->Add(c2);
  t1.root()->Add(c3);

  auto damage = DiffLayerTree(t1, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 250, 50));

  MockLayerTree t2;
  t2.root()->Add(c1);
  t2.root()->Add(c2);
  t2.root()->Add(c3);

  damage = DiffLayerTree(t2, t1);
  EXPECT_TRUE(damage.frame_damage.IsEmpty());

  MockLayerTree t3;
  t3.root()->Add(CreateContainerLayer({std::make_shared<MockLayer>(path1a)}));
  t3.root()->Add(c2);
  t3.root()->Add(c3);

  damage = DiffLayerTree(t3, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 50, 150));

  MockLayerTree t4;
  t4.root()->Add(c1);
  t4.root()->Add(CreateContainerLayer(std::make_shared<MockLayer>(path2a)));
  t4.root()->Add(c3);

  damage = DiffLayerTree(t4, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(100, 0, 150, 150));

  MockLayerTree t5;
  t5.root()->Add(c1);
  t5.root()->Add(c2);
  t5.root()->Add(CreateContainerLayer(std::make_shared<MockLayer>(path3a)));

  damage = DiffLayerTree(t5, t1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(200, 0, 250, 150));
}

}  // namespace testing
}  // namespace clay
