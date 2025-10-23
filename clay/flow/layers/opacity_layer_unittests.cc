// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/compositor/compositor_state.h"
#include "clay/flow/layers/clip_rect_layer.h"
#include "clay/flow/layers/image_filter_layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/layers/platform_view_layer.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/flow/testing/diff_context_test.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/gfx/testing_utils.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPaint.h"

namespace clay {
namespace testing {

using OpacityLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(OpacityLayerTest, LeafLayer) {
  auto layer =
      std::make_shared<OpacityLayer>(SK_AlphaOPAQUE, skity::Vec2(0.0f, 0.0f));

  EXPECT_DEATH_IF_SUPPORTED(layer->Preroll(preroll_context()),
                            "\\!layers\\(\\)\\.empty\\(\\)");
}

TEST_F(OpacityLayerTest, PaintingEmptyLayerDies) {
  auto mock_layer = std::make_shared<MockLayer>(SkPath());
  auto layer =
      std::make_shared<OpacityLayer>(SK_AlphaOPAQUE, skity::Vec2(0.0f, 0.0f));
  layer->Add(mock_layer);

  layer->Preroll(preroll_context());
  EXPECT_EQ(mock_layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(SkPath().getBounds()));
  EXPECT_EQ(layer->paint_bounds(), mock_layer->paint_bounds());
  EXPECT_EQ(layer->child_paint_bounds(), mock_layer->paint_bounds());
  EXPECT_FALSE(mock_layer->needs_painting(paint_context()));
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(OpacityLayerTest, PaintBeforePrerollDies) {
  SkPath child_path;
  child_path.addRect(5.0f, 6.0f, 20.5f, 21.5f);
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer =
      std::make_shared<OpacityLayer>(SK_AlphaOPAQUE, skity::Vec2(0.0f, 0.0f));
  layer->Add(mock_layer);

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(OpacityLayerTest, TranslateChildren) {
  SkPath child_path1;
  child_path1.addRect(10.0f, 10.0f, 20.0f, 20.f);
  SkPaint child_paint1(SkColors::kGray);
  auto layer = std::make_shared<OpacityLayer>(0.5, skity::Vec2(10, 10));
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  layer->Add(mock_layer1);

  auto initial_transform = SkMatrix::Scale(2.0, 2.0);
  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());

  skity::Rect layer_bounds = mock_layer1->paint_bounds();
  mock_layer1->parent_matrix().MapRect(&layer_bounds, layer_bounds);

  EXPECT_EQ(layer_bounds, skity::Rect::MakeXYWH(40, 40, 20, 20));
}

TEST_F(OpacityLayerTest, CacheChild) {
  const SkAlpha alpha_half = 255 / 2;
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  auto other_transform = SkMatrix::Scale(1.0, 2.0);
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer =
      std::make_shared<OpacityLayer>(alpha_half, skity::Vec2(0.0f, 0.0f));
  layer->Add(mock_layer);
  SkPaint paint;

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);
  SkCanvas other_canvas;
  other_canvas.setMatrix(other_transform);

  use_mock_raster_cache();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);

  const auto* cacheable_opacity_item = layer->raster_cache_item();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(cacheable_opacity_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_opacity_item->GetId().has_value());

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)1);

  EXPECT_EQ(cacheable_opacity_item->cache_state(),
            RasterCacheItem::CacheState::kChildren);
  EXPECT_EQ(
      cacheable_opacity_item->GetId().value(),
      RasterCacheKeyID(RasterCacheKeyID::LayerChildrenIds(layer.get()).value(),
                       RasterCacheKeyType::kLayerChildren));
  EXPECT_FALSE(raster_cache()->Draw(cacheable_opacity_item->GetId().value(),
                                    other_canvas, &paint));
  EXPECT_TRUE(raster_cache()->Draw(cacheable_opacity_item->GetId().value(),
                                   cache_canvas, &paint));
}

TEST_F(OpacityLayerTest, CacheChildren) {
  const SkAlpha alpha_half = 255 / 2;
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  auto other_transform = SkMatrix::Scale(1.0, 2.0);
  const SkPath child_path1 = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  const SkPath child_path2 = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  SkPaint paint;
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2);
  auto layer =
      std::make_shared<OpacityLayer>(alpha_half, skity::Vec2(0.0f, 0.0f));
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);
  SkCanvas other_canvas;
  other_canvas.setMatrix(other_transform);

  use_mock_raster_cache();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);

  const auto* cacheable_opacity_item = layer->raster_cache_item();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(cacheable_opacity_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_opacity_item->GetId().has_value());

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)1);

  EXPECT_EQ(cacheable_opacity_item->cache_state(),
            RasterCacheItem::CacheState::kChildren);
  EXPECT_EQ(
      cacheable_opacity_item->GetId().value(),
      RasterCacheKeyID(RasterCacheKeyID::LayerChildrenIds(layer.get()).value(),
                       RasterCacheKeyType::kLayerChildren));
  EXPECT_FALSE(raster_cache()->Draw(cacheable_opacity_item->GetId().value(),
                                    other_canvas, &paint));
  EXPECT_TRUE(raster_cache()->Draw(cacheable_opacity_item->GetId().value(),
                                   cache_canvas, &paint));
}

TEST_F(OpacityLayerTest, ShouldNotCacheChildren) {
  SkPaint paint;
  auto opacity_layer = std::make_shared<OpacityLayer>(128, skity::Vec2(20, 20));
  auto mock_layer = MockLayer::MakeOpacityCompatible(SkPath());
  opacity_layer->Add(mock_layer);

  PrerollContext* context = preroll_context();

  use_mock_raster_cache();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);

  const auto* cacheable_opacity_item = opacity_layer->raster_cache_item();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(cacheable_opacity_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_opacity_item->GetId().has_value());

  opacity_layer->Preroll(preroll_context());

  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
  EXPECT_TRUE(opacity_layer->children_can_accept_opacity());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(cacheable_opacity_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_opacity_item->Draw(paint_context(), &paint));
}

TEST_F(OpacityLayerTest, FullyOpaque) {
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  const skity::Vec2 layer_offset = skity::Vec2(0.5f, 1.5f);
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 0.5f);
  const skity::Matrix layer_transform =
      skity::Matrix::Translate(layer_offset.x, layer_offset.y);
  const SkPaint child_paint = SkPaint(SkColors::kGreen);
  skity::Rect expected_layer_bounds;
  layer_transform.MapRect(
      &expected_layer_bounds,
      clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<OpacityLayer>(SK_AlphaOPAQUE, layer_offset);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(mock_layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_layer_bounds);
  EXPECT_EQ(layer->child_paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform * layer_transform);
  EXPECT_EQ(mock_layer->parent_mutators(),
            std::vector({Mutator(layer_transform)}));

  const SkPaint opacity_paint = SkPaint(SkColors::kBlack);  // A = 1.0f
  skity::Rect opacity_bounds =
      expected_layer_bounds.MakeOffset(-layer_offset.x, -layer_offset.y);
  opacity_bounds.RoundOut();
  auto expected_draw_calls = std::vector(
      {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
       MockCanvas::DrawCall{
           1, MockCanvas::ConcatMatrixData{clay::ConvertSkityMatrixToSkM44(
                  layer_transform)}},
       // NOLINTNEXTLINE
       MockCanvas::DrawCall{
           // NOLINTNEXTLINE
           1, MockCanvas::DrawPathData{child_path, child_paint}},
       MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}});
  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(), expected_draw_calls);
}

TEST_F(OpacityLayerTest, FullyTransparent) {
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  const skity::Vec2 layer_offset = skity::Vec2(0.5f, 1.5f);
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 0.5f);
  const skity::Matrix layer_transform =
      skity::Matrix::Translate(layer_offset.x, layer_offset.y);
  const SkPaint child_paint = SkPaint(SkColors::kGreen);
  skity::Rect expected_layer_bounds;
  layer_transform.MapRect(
      &expected_layer_bounds,
      clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer =
      std::make_shared<OpacityLayer>(SK_AlphaTRANSPARENT, layer_offset);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(mock_layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_layer_bounds);
  EXPECT_EQ(layer->child_paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform * layer_transform);
  EXPECT_EQ(
      mock_layer->parent_mutators(),
      std::vector({Mutator(layer_transform), Mutator(SK_AlphaTRANSPARENT)}));

  auto expected_draw_calls = std::vector(
      {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
       MockCanvas::DrawCall{
           1, MockCanvas::ConcatMatrixData{clay::ConvertSkityMatrixToSkM44(
                  layer_transform)}},
       MockCanvas::DrawCall{1, MockCanvas::SaveData{2}},
       MockCanvas::DrawCall{
           2,
           MockCanvas::ClipRectData{clay::ConvertSkityRectToSkRect(kEmptyRect),
                                    SkClipOp::kIntersect,
                                    MockCanvas::kHard_ClipEdgeStyle}},
       MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
       MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}});
  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(), expected_draw_calls);
}

TEST_F(OpacityLayerTest, HalfTransparent) {
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  const skity::Vec2 layer_offset = skity::Vec2(0.5f, 1.5f);
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 0.5f);
  const skity::Matrix layer_transform =
      skity::Matrix::Translate(layer_offset.x, layer_offset.y);
  const SkPaint child_paint = SkPaint(SkColors::kGreen);
  skity::Rect expected_layer_bounds;
  layer_transform.MapRect(
      &expected_layer_bounds,
      clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  const SkAlpha alpha_half = 255 / 2;
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<OpacityLayer>(alpha_half, layer_offset);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(mock_layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_layer_bounds);
  EXPECT_EQ(layer->child_paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path.getBounds()));
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform * layer_transform);
  EXPECT_EQ(mock_layer->parent_mutators(),
            std::vector({Mutator(layer_transform), Mutator(alpha_half)}));

  skity::Rect opacity_bounds =
      expected_layer_bounds.MakeOffset(-layer_offset.x, -layer_offset.y);
  opacity_bounds.RoundOut();
  DlPaint save_paint = DlPaint().setAlpha(alpha_half);
  DlPaint child_dl_paint = DlPaint().setColor(DlColor::kGreen());

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  auto canvas = recorder.beginRecording(kDlBounds, rtree.get());
  auto bounds_tmp = clay::ConvertSkityRectToSkRect(opacity_bounds);
  SkPaint sk_paint = save_paint.gr_object();
  canvas->save();
  canvas->translate(layer_offset.x, layer_offset.y);
  canvas->saveLayer(&bounds_tmp, &sk_paint);
  canvas->drawPath(child_path, child_dl_paint.gr_object());
  canvas->restore();
  canvas->restore();
  auto expected_display_list = recorder.finishRecordingAsPicture();

  layer->Paint(display_list_paint_context());
  EXPECT_TRUE(display_list()->approximateOpCount() ==
              expected_display_list->approximateOpCount());
}

TEST_F(OpacityLayerTest, Nested) {
  const SkPath child1_path = SkPath().addRect(SkRect::MakeWH(5.0f, 6.0f));
  const SkPath child2_path = SkPath().addRect(SkRect::MakeWH(2.0f, 7.0f));
  const SkPath child3_path = SkPath().addRect(SkRect::MakeWH(6.0f, 6.0f));
  const skity::Vec2 layer1_offset = skity::Vec2(0.5f, 1.5f);
  const skity::Vec2 layer2_offset = skity::Vec2(2.5f, 0.5f);
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 0.5f);
  const skity::Matrix layer1_transform =
      skity::Matrix::Translate(layer1_offset.x, layer1_offset.y);
  const skity::Matrix layer2_transform =
      skity::Matrix::Translate(layer2_offset.x, layer2_offset.y);
  const SkPaint child1_paint = SkPaint(SkColors::kRed);
  const SkPaint child2_paint = SkPaint(SkColors::kBlue);
  const SkPaint child3_paint = SkPaint(SkColors::kGreen);
  const SkAlpha alpha1 = 155;
  const SkAlpha alpha2 = 224;
  auto mock_layer1 = std::make_shared<MockLayer>(child1_path, child1_paint);
  auto mock_layer2 = std::make_shared<MockLayer>(child2_path, child2_paint);
  auto mock_layer3 = std::make_shared<MockLayer>(child3_path, child3_paint);
  auto layer1 = std::make_shared<OpacityLayer>(alpha1, layer1_offset);
  auto layer2 = std::make_shared<OpacityLayer>(alpha2, layer2_offset);
  layer2->Add(mock_layer2);
  layer1->Add(mock_layer1);
  layer1->Add(layer2);
  layer1->Add(mock_layer3);  // Ensure something is processed after recursion

  skity::Rect expected_layer2_bounds;
  layer2_transform.MapRect(
      &expected_layer2_bounds,
      clay::ConvertSkRectToSkityRect(child2_path.getBounds()));
  skity::Rect layer1_child_bounds = expected_layer2_bounds;
  layer1_child_bounds.Join(
      clay::ConvertSkRectToSkityRect(child1_path.getBounds()));
  layer1_child_bounds.Join(
      clay::ConvertSkRectToSkityRect(child3_path.getBounds()));
  skity::Rect expected_layer1_bounds =
      layer1_transform.MapRect(layer1_child_bounds);
  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer1->Preroll(preroll_context());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child1_path.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child2_path.getBounds()));
  EXPECT_EQ(mock_layer3->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child3_path.getBounds()));
  EXPECT_EQ(layer1->paint_bounds(), expected_layer1_bounds);
  EXPECT_EQ(layer1->child_paint_bounds(), layer1_child_bounds);
  EXPECT_EQ(layer2->paint_bounds(), expected_layer2_bounds);
  EXPECT_EQ(layer2->child_paint_bounds(),
            clay::ConvertSkRectToSkityRect(child2_path.getBounds()));
  EXPECT_TRUE(mock_layer1->needs_painting(paint_context()));
  EXPECT_TRUE(mock_layer2->needs_painting(paint_context()));
  EXPECT_TRUE(mock_layer3->needs_painting(paint_context()));
  EXPECT_TRUE(layer1->needs_painting(paint_context()));
  EXPECT_TRUE(layer2->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer1->parent_matrix(), initial_transform * layer1_transform);
  //   EXPECT_EQ(mock_layer1->parent_mutators(),
  //             std::vector({Mutator(layer1_transform), Mutator(alpha1)}));
  EXPECT_EQ(mock_layer2->parent_matrix(),
            (initial_transform * layer1_transform) * layer2_transform);
  //   EXPECT_EQ(mock_layer2->parent_mutators(),
  //             std::vector({Mutator(layer1_transform), Mutator(alpha1),
  //                          Mutator(layer2_transform), Mutator(alpha2)}));
  EXPECT_EQ(mock_layer3->parent_matrix(), initial_transform * layer1_transform);
  //   EXPECT_EQ(mock_layer3->parent_mutators(),
  //             std::vector({Mutator(layer1_transform), Mutator(alpha1)}));

  SkPaint opacity1_paint;
  opacity1_paint.setAlphaf(alpha1 * (1.0 / SK_AlphaOPAQUE));
  SkPaint opacity2_paint;
  opacity2_paint.setAlphaf(alpha2 * (1.0 / SK_AlphaOPAQUE));
  skity::Rect opacity1_bounds =
      expected_layer1_bounds.MakeOffset(-layer1_offset.x, -layer1_offset.y);
  skity::Rect opacity2_bounds =
      expected_layer2_bounds.MakeOffset(-layer2_offset.x, -layer2_offset.y);
  auto expected_draw_calls = std::vector(
      {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
       MockCanvas::DrawCall{
           1, MockCanvas::ConcatMatrixData{clay::ConvertSkityMatrixToSkM44(
                  layer1_transform)}},
       MockCanvas::DrawCall{1,
                            MockCanvas::SaveLayerData{
                                clay::ConvertSkityRectToSkRect(opacity1_bounds),
                                opacity1_paint, nullptr, 2}},
       MockCanvas::DrawCall{
           2, MockCanvas::DrawPathData{child1_path, child1_paint}},
       MockCanvas::DrawCall{2, MockCanvas::SaveData{3}},
       MockCanvas::DrawCall{
           3, MockCanvas::ConcatMatrixData{clay::ConvertSkityMatrixToSkM44(
                  layer2_transform)}},
       MockCanvas::DrawCall{3,
                            MockCanvas::SaveLayerData{
                                clay::ConvertSkityRectToSkRect(opacity2_bounds),
                                opacity2_paint, nullptr, 4}},
       MockCanvas::DrawCall{
           4, MockCanvas::DrawPathData{child2_path, child2_paint}},
       MockCanvas::DrawCall{4, MockCanvas::RestoreData{3}},
       MockCanvas::DrawCall{3, MockCanvas::RestoreData{2}},
       MockCanvas::DrawCall{
           2, MockCanvas::DrawPathData{child3_path, child3_paint}},
       MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
       MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}});
  layer1->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(), expected_draw_calls);
}

TEST_F(OpacityLayerTest, Readback) {
  auto layer =
      std::make_shared<OpacityLayer>(kOpaque_SkAlphaType, skity::Vec2());
  layer->Add(std::make_shared<MockLayer>(SkPath()));

  // OpacityLayer does not read from surface
  preroll_context()->surface_needs_readback = false;
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);

  // OpacityLayer blocks child with readback
  auto mock_layer = std::make_shared<MockLayer>(SkPath(), SkPaint());
  mock_layer->set_fake_reads_surface(true);
  layer->Add(mock_layer);
  preroll_context()->surface_needs_readback = false;
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);
}

TEST_F(OpacityLayerTest, CullRectIsTransformed) {
  auto clip_rect_layer = std::make_shared<ClipRectLayer>(
      skity::Rect::MakeLTRB(0, 0, 10, 10), clay::hardEdge);
  auto opacity_layer = std::make_shared<OpacityLayer>(128, skity::Vec2(20, 20));
  auto mock_layer = std::make_shared<MockLayer>(SkPath());
  clip_rect_layer->Add(opacity_layer);
  opacity_layer->Add(mock_layer);
  clip_rect_layer->Preroll(preroll_context());
  EXPECT_EQ(mock_layer->parent_cull_rect().Left(), -20);
  EXPECT_EQ(mock_layer->parent_cull_rect().Top(), -20);
}

TEST_F(OpacityLayerTest, OpacityInheritanceCompatibleChild) {
  auto opacity_layer = std::make_shared<OpacityLayer>(128, skity::Vec2(20, 20));
  auto mock_layer = MockLayer::MakeOpacityCompatible(SkPath());
  opacity_layer->Add(mock_layer);

  PrerollContext* context = preroll_context();
  opacity_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
  EXPECT_TRUE(opacity_layer->children_can_accept_opacity());
}

TEST_F(OpacityLayerTest, OpacityInheritanceIncompatibleChild) {
  auto opacity_layer = std::make_shared<OpacityLayer>(128, skity::Vec2(20, 20));
  auto mock_layer = MockLayer::Make(SkPath());
  opacity_layer->Add(mock_layer);

  PrerollContext* context = preroll_context();
  opacity_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
  EXPECT_FALSE(opacity_layer->children_can_accept_opacity());
}

TEST_F(OpacityLayerTest, OpacityInheritanceThroughContainer) {
  auto opacity_layer = std::make_shared<OpacityLayer>(128, skity::Vec2(20, 20));
  auto container_layer = std::make_shared<ContainerLayer>();
  auto mock_layer = MockLayer::MakeOpacityCompatible(SkPath());
  container_layer->Add(mock_layer);
  opacity_layer->Add(container_layer);

  PrerollContext* context = preroll_context();
  opacity_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
  EXPECT_TRUE(opacity_layer->children_can_accept_opacity());
}

TEST_F(OpacityLayerTest, OpacityInheritanceThroughTransform) {
  auto opacity_layer = std::make_shared<OpacityLayer>(128, skity::Vec2(20, 20));
  auto transformLayer =
      std::make_shared<TransformLayer>(skity::Matrix::Scale(2, 2));
  auto mock_layer = MockLayer::MakeOpacityCompatible(SkPath());
  transformLayer->Add(mock_layer);
  opacity_layer->Add(transformLayer);

  PrerollContext* context = preroll_context();
  opacity_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
  EXPECT_TRUE(opacity_layer->children_can_accept_opacity());
}

TEST_F(OpacityLayerTest, OpacityInheritanceThroughImageFilter) {
  auto opacity_layer = std::make_shared<OpacityLayer>(128, skity::Vec2(20, 20));
  auto filter_layer = std::make_shared<ImageFilterLayer>(
      std::make_shared<DlBlurImageFilter>(5.0, 5.0, DlTileMode::kClamp));
  auto mock_layer = MockLayer::MakeOpacityCompatible(SkPath());
  filter_layer->Add(mock_layer);
  opacity_layer->Add(filter_layer);

  PrerollContext* context = preroll_context();
  opacity_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
  EXPECT_TRUE(opacity_layer->children_can_accept_opacity());
}

TEST_F(OpacityLayerTest, OpacityInheritanceNestedWithCompatibleChild) {
  skity::Vec2 offset1 = skity::Vec2(10, 20);
  skity::Vec2 offset2 = skity::Vec2(20, 10);
  SkPath mock_path = SkPath::Rect({10, 10, 20, 20});
  auto opacity_layer_1 = std::make_shared<OpacityLayer>(128, offset1);
  auto opacity_layer_2 = std::make_shared<OpacityLayer>(64, offset2);
  auto mock_layer = MockLayer::MakeOpacityCompatible(mock_path);
  opacity_layer_2->Add(mock_layer);
  opacity_layer_1->Add(opacity_layer_2);

  PrerollContext* context = preroll_context();
  opacity_layer_1->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
  EXPECT_TRUE(opacity_layer_1->children_can_accept_opacity());
  EXPECT_TRUE(opacity_layer_2->children_can_accept_opacity());

  // cspell:words savelayer_paint
  SkPaint savelayer_paint;
  float inherited_opacity = 128 * 1.0 / SK_AlphaOPAQUE;
  inherited_opacity *= 64 * 1.0 / SK_AlphaOPAQUE;
  savelayer_paint.setAlphaf(inherited_opacity);

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    canvas->save();
    {
      canvas->translate(offset1.x, offset1.y);
      {
        canvas->save();
        {
          canvas->translate(offset2.x, offset2.y);
          {
            SkPaint sk_paint;
            sk_paint.setAlpha(inherited_opacity * 255.f);
            canvas->drawPath(mock_path, sk_paint);
          }
        }
        canvas->restore();
      }
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  opacity_layer_1->Paint(display_list_paint_context());
  EXPECT_TRUE(expected_display_list->approximateOpCount() ==
              display_list()->approximateOpCount());
}

TEST_F(OpacityLayerTest, OpacityInheritanceNestedWithIncompatibleChild) {
  skity::Vec2 offset1 = skity::Vec2(10, 20);
  skity::Vec2 offset2 = skity::Vec2(20, 10);
  SkPath mock_path = SkPath::Rect({10, 10, 20, 20});
  auto opacity_layer_1 = std::make_shared<OpacityLayer>(128, offset1);
  auto opacity_layer_2 = std::make_shared<OpacityLayer>(64, offset2);
  auto mock_layer = MockLayer::Make(mock_path);
  opacity_layer_2->Add(mock_layer);
  opacity_layer_1->Add(opacity_layer_2);

  PrerollContext* context = preroll_context();
  opacity_layer_1->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
  EXPECT_TRUE(opacity_layer_1->children_can_accept_opacity());
  EXPECT_FALSE(opacity_layer_2->children_can_accept_opacity());

  SkPaint savelayer_paint;
  float inherited_opacity = 128 * 1.0 / SK_AlphaOPAQUE;
  inherited_opacity *= 64 * 1.0 / SK_AlphaOPAQUE;
  savelayer_paint.setAlphaf(inherited_opacity);

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    canvas->save();
    {
      canvas->translate(offset1.x, offset1.y);
      {
        canvas->save();
        {
          canvas->translate(offset2.x, offset2.y);
          SkPaint sk_paint;
          sk_paint.setColor(savelayer_paint.getAlpha() << 24);
          auto bounds_tmp =
              clay::ConvertSkityRectToSkRect(mock_layer->paint_bounds());
          canvas->saveLayer(&bounds_tmp, &sk_paint);
          {
            SkPaint new_paint;
            new_paint.setColor(0xFF000000);
            canvas->drawPath(mock_path, new_paint);
          }
          canvas->restore();
        }
        canvas->restore();
      }
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  opacity_layer_1->Paint(display_list_paint_context());
  EXPECT_TRUE(expected_display_list->approximateOpCount() ==
              display_list()->approximateOpCount());
}

using OpacityLayerDiffTest = DiffContextTest;

TEST_F(OpacityLayerDiffTest, FractionalTranslation) {
  auto picture = CreatePictureLayer(
      CreatePicture(skity::Rect::MakeLTRB(10, 10, 60, 60), 1));
  auto layer = CreateOpacityLater({picture}, 128, skity::Vec2(0.5, 0.5));

  MockLayerTree tree1;
  tree1.root()->Add(layer);

  auto damage = DiffLayerTree(tree1, MockLayerTree(), skity::Rect::MakeEmpty(),
                              0, 0, /*use_raster_cache=*/false);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(10, 10, 61, 61));
}

#ifndef SUPPORT_FRACTIONAL_TRANSLATION
TEST_F(OpacityLayerDiffTest, FractionalTranslationWithRasterCache) {
  auto picture = CreateDisplayListLayer(
      CreateDisplayList(skity::Rect::MakeLTRB(10, 10, 60, 60), 1));
  auto layer = CreateOpacityLater({picture}, 128, skity::Vec2(0.5, 0.5));

  MockLayerTree tree1;
  tree1.root()->Add(layer);

  auto damage = DiffLayerTree(tree1, MockLayerTree(), skity::Rect::MakeEmpty(),
                              0, 0, /*use_raster_cache=*/true);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(11, 11, 61, 61));
}

TEST_F(OpacityLayerTest, FullyOpaqueWithFractionalValues) {
  use_mock_raster_cache();  // Ensure non-fractional alignment.

  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  const skity::Vec2 layer_offset = skity::Vec2(0.5f, 1.5f);
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 0.5f);
  const skity::Matrix layer_transform =
      skity::Matrix::Translate(layer_offset.x, layer_offset.y);
  const SkPaint child_paint = SkPaint(SkColors::kGreen);
  const skity::Rect expected_layer_bounds =
      layer_transform.MapRect(child_path.getBounds());
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<OpacityLayer>(SK_AlphaOPAQUE, layer_offset);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());

  const SkPaint opacity_paint = SkPaint(SkColors::kBlack);  // A = 1.0f
  skity::Rect opacity_bounds;
  expected_layer_bounds.makeOffset(-layer_offset.x, -layer_offset.y)
      .RoundOut(&opacity_bounds);
  auto expected_draw_calls = std::vector(
      {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
       MockCanvas::DrawCall{
           1, MockCanvas::ConcatMatrixData{skity::Matrix(layer_transform)}},
       MockCanvas::DrawCall{
           1, MockCanvas::SetMatrixData{skity::Matrix(
                  RasterCacheUtil::GetIntegralTransCTM(layer_transform))}},
       MockCanvas::DrawCall{1,
                            MockCanvas::DrawPathData{child_path, child_paint}},
       MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}});
  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(), expected_draw_calls);
}
#endif

TEST_F(OpacityLayerTest, FullyTransparentDoesNotCullPlatformView) {
  const skity::Vec2 opacity_offset = skity::Vec2(0.5f, 1.5f);
  const skity::Vec2 view_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 view_size = skity::Vec2(8.0f, 8.0f);
  const int64_t view_id = 42;
  auto platform_view =
      std::make_shared<PlatformViewLayer>(view_offset, view_size, view_id);

  auto opacity =
      std::make_shared<OpacityLayer>(SK_AlphaTRANSPARENT, opacity_offset);
  opacity->Add(platform_view);

  CompositorState compositor_state({64, 64});
  preroll_context()->compositor_state = &compositor_state;
  paint_context().compositor_state = &compositor_state;

  opacity->Preroll(preroll_context());
  // cspell:words prerolled
  EXPECT_EQ(compositor_state.GetCompositionOrder(),
            std::vector<int64_t>({view_id}));

  opacity->Paint(paint_context());
  EXPECT_EQ(paint_context().canvas,
            compositor_state.GetSlices()[view_id]->canvas());
}

}  // namespace testing
}  // namespace clay
