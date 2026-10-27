// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/compositor_context.h"
#include "clay/flow/layers/color_filter_layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/raster_cache_key.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/paint.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/gfx/testing_utils.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/effects/SkColorMatrixFilter.h"
namespace clay {
namespace testing {

using ColorFilterLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(ColorFilterLayerTest, PaintingEmptyLayerDies) {
  auto layer = std::make_shared<ColorFilterLayer>(
      std::make_shared<DlUnknownColorFilter>(sk_sp<SkColorFilter>()));

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(ColorFilterLayerTest, PaintBeforePrerollDies) {
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  auto mock_layer = std::make_shared<MockLayer>(child_path);

  auto layer = std::make_shared<ColorFilterLayer>(
      std::make_shared<DlUnknownColorFilter>(sk_sp<SkColorFilter>()));
  layer->Add(mock_layer);

  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(ColorFilterLayerTest, EmptyFilter) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ColorFilterLayer>(nullptr);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  SkPaint filter_paint;
  filter_paint.setColorFilter(nullptr);
  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({
                MockCanvas::DrawCall{
                    0, MockCanvas::DrawPathData{child_path, child_paint}},
            }));
}

TEST_F(ColorFilterLayerTest, SimpleFilter) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);

  auto dl_color_filter = DlLinearToSrgbGammaColorFilter::instance;
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ColorFilterLayer>(dl_color_filter);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    SkPaint paint;
    paint.setColorFilter(dl_color_filter->gr_object());
    auto bounds_tmp = clay::ConvertSkityRectToSkRect(child_bounds);
    canvas->saveLayer(bounds_tmp, &paint);
    {
      SkPaint tmp_paint;
      tmp_paint.setColor(SK_ColorYELLOW);
      canvas->drawPath(child_path, tmp_paint);
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  layer->Paint(display_list_paint_context());
  EXPECT_TRUE(display_list()->approximateOpCount() ==
              expected_display_list->approximateOpCount());
}

TEST_F(ColorFilterLayerTest, MultipleChildren) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 2.5f, 3.5f);
  const SkPath child_path1 =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath child_path2 = SkPath().addRect(
      clay::ConvertSkityRectToSkRect(child_bounds.MakeOffset(3.0f, 0.0f)));
  const SkPaint child_paint1 = SkPaint(SkColors::kYellow);
  const SkPaint child_paint2 = SkPaint(SkColors::kCyan);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto dl_color_filter = DlSrgbToLinearGammaColorFilter::instance;
  auto layer = std::make_shared<ColorFilterLayer>(dl_color_filter);
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  skity::Rect children_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  children_bounds.Join(clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), children_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), children_bounds);
  EXPECT_TRUE(mock_layer1->needs_painting(paint_context()));
  EXPECT_TRUE(mock_layer2->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer1->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer2->parent_matrix(), initial_transform);

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    SkPaint paint;
    paint.setColorFilter(dl_color_filter->gr_object());
    auto bounds_tmp = clay::ConvertSkityRectToSkRect(children_bounds);
    canvas->saveLayer(bounds_tmp, &paint);
    {
      SkPaint tmp_paint;
      tmp_paint.setColor(SK_ColorYELLOW);
      canvas->drawPath(child_path1, tmp_paint);
    }
    {
      SkPaint tmp_paint;
      tmp_paint.setColor(SK_ColorCYAN);
      canvas->drawPath(child_path2, tmp_paint);
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  layer->Paint(display_list_paint_context());
  EXPECT_TRUE(display_list()->approximateOpCount() ==
              expected_display_list->approximateOpCount());
}

TEST_F(ColorFilterLayerTest, Nested) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 2.5f, 3.5f);
  const SkPath child_path1 =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath child_path2 = SkPath().addRect(
      clay::ConvertSkityRectToSkRect(child_bounds.MakeOffset(3.0f, 0.0f)));
  const SkPaint child_paint1 = SkPaint(SkColors::kYellow);
  const SkPaint child_paint2 = SkPaint(SkColors::kCyan);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto dl_color_filter = DlSrgbToLinearGammaColorFilter::instance;
  auto layer1 = std::make_shared<ColorFilterLayer>(dl_color_filter);

  auto layer2 = std::make_shared<ColorFilterLayer>(dl_color_filter);
  layer2->Add(mock_layer2);
  layer1->Add(mock_layer1);
  layer1->Add(layer2);

  skity::Rect children_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  children_bounds.Join(clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer1->Preroll(preroll_context());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer1->paint_bounds(), children_bounds);
  EXPECT_EQ(layer1->child_paint_bounds(), children_bounds);
  EXPECT_EQ(layer2->paint_bounds(), mock_layer2->paint_bounds());
  EXPECT_EQ(layer2->child_paint_bounds(), mock_layer2->paint_bounds());
  EXPECT_TRUE(mock_layer1->needs_painting(paint_context()));
  EXPECT_TRUE(mock_layer2->needs_painting(paint_context()));
  EXPECT_TRUE(layer1->needs_painting(paint_context()));
  EXPECT_TRUE(layer2->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer1->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer2->parent_matrix(), initial_transform);

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    SkPaint paint;
    paint.setColorFilter(dl_color_filter->gr_object());
    auto bounds_tmp = clay::ConvertSkityRectToSkRect(children_bounds);
    canvas->saveLayer(bounds_tmp, &paint);
    {
      SkPaint tmp_paint;
      tmp_paint.setColor(SK_ColorYELLOW);
      canvas->drawPath(child_path1, tmp_paint);
    }
    {
      SkPaint tmp_paint;
      tmp_paint.setColor(SK_ColorBLACK);
      tmp_paint.setColorFilter(dl_color_filter->gr_object());
      canvas->saveLayer(&child_path2.getBounds(), &tmp_paint);
      {
        SkPaint inner_paint;
        inner_paint.setColor(SK_ColorCYAN);
        canvas->drawPath(child_path2, inner_paint);
      }
      canvas->restore();
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  layer1->Paint(display_list_paint_context());
  EXPECT_TRUE(display_list()->approximateOpCount() ==
              expected_display_list->approximateOpCount());
}

TEST_F(ColorFilterLayerTest, Readback) {
  auto initial_transform = skity::Matrix();

  // ColorFilterLayer does not read from surface
  auto layer = std::make_shared<ColorFilterLayer>(
      DlLinearToSrgbGammaColorFilter::instance);
  preroll_context()->surface_needs_readback = false;
  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);

  // ColorFilterLayer blocks child with readback
  auto mock_layer = std::make_shared<MockLayer>(SkPath(), SkPaint());
  mock_layer->set_fake_reads_surface(true);
  layer->Add(mock_layer);
  preroll_context()->surface_needs_readback = false;
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);
}

TEST_F(ColorFilterLayerTest, CacheChild) {
  auto layer_filter = DlSrgbToLinearGammaColorFilter::instance;
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  auto other_transform = SkMatrix::Scale(1.0, 2.0);
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  SkPaint paint = SkPaint();
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ColorFilterLayer>(layer_filter);
  layer->Add(mock_layer);

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);
  SkCanvas other_canvas;
  other_canvas.setMatrix(other_transform);

  use_mock_raster_cache();
  const auto* cacheable_color_filter_item = layer->raster_cache_item();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(cacheable_color_filter_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_color_filter_item->GetId().has_value());

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)1);
  EXPECT_EQ(cacheable_color_filter_item->cache_state(),
            RasterCacheItem::CacheState::kChildren);
  EXPECT_EQ(
      cacheable_color_filter_item->GetId().value(),
      RasterCacheKeyID(RasterCacheKeyID::LayerChildrenIds(layer.get()).value(),
                       RasterCacheKeyType::kLayerChildren));
  EXPECT_FALSE(raster_cache()->Draw(
      cacheable_color_filter_item->GetId().value(), other_canvas, &paint));
  EXPECT_TRUE(raster_cache()->Draw(cacheable_color_filter_item->GetId().value(),
                                   cache_canvas, &paint));
}

TEST_F(ColorFilterLayerTest, CacheChildren) {
  auto layer_filter = DlSrgbToLinearGammaColorFilter::instance;
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  auto other_transform = SkMatrix::Scale(1.0, 2.0);
  const SkPath child_path1 = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  const SkPath child_path2 = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2);
  auto layer = std::make_shared<ColorFilterLayer>(layer_filter);
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);
  SkPaint paint = SkPaint();

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);
  SkCanvas other_canvas;
  other_canvas.setMatrix(other_transform);

  use_mock_raster_cache();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  const auto* cacheable_color_filter_item = layer->raster_cache_item();
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);

  EXPECT_EQ(cacheable_color_filter_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_color_filter_item->GetId().has_value());

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)1);
  EXPECT_EQ(cacheable_color_filter_item->cache_state(),
            RasterCacheItem::CacheState::kChildren);
  EXPECT_EQ(
      cacheable_color_filter_item->GetId().value(),
      RasterCacheKeyID(RasterCacheKeyID::LayerChildrenIds(layer.get()).value(),
                       RasterCacheKeyType::kLayerChildren));
  EXPECT_FALSE(raster_cache()->Draw(
      cacheable_color_filter_item->GetId().value(), other_canvas, &paint));
  EXPECT_TRUE(raster_cache()->Draw(cacheable_color_filter_item->GetId().value(),
                                   cache_canvas, &paint));
}

TEST_F(ColorFilterLayerTest, CacheColorFilterLayerSelf) {
  auto layer_filter = DlSrgbToLinearGammaColorFilter::instance;
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  auto other_transform = SkMatrix::Scale(1.0, 2.0);
  const SkPath child_path1 = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  const SkPath child_path2 = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2);
  auto layer = std::make_shared<ColorFilterLayer>(layer_filter);
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);
  SkPaint paint = SkPaint();

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);
  SkCanvas other_canvas;
  other_canvas.setMatrix(other_transform);

  use_mock_raster_cache();
  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  const auto* cacheable_color_filter_item = layer->raster_cache_item();

  // frame 1.
  layer->Preroll(preroll_context());
  layer->Paint(paint_context());
  // frame 2.
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());
  // ColorFilterLayer default cache children.
  EXPECT_EQ(cacheable_color_filter_item->cache_state(),
            RasterCacheItem::CacheState::kChildren);
  EXPECT_TRUE(raster_cache()->Draw(cacheable_color_filter_item->GetId().value(),
                                   cache_canvas, &paint));
  EXPECT_FALSE(raster_cache()->Draw(
      cacheable_color_filter_item->GetId().value(), other_canvas, &paint));
  layer->Paint(paint_context());

  // frame 3.
  layer->Preroll(preroll_context());
  layer->Paint(paint_context());

  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());
  // frame1,2 cache the ColorFilterLayer's children layer, frame3 cache the
  // ColorFilterLayer
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)2);

  // ColorFilterLayer default cache itself.
  EXPECT_EQ(cacheable_color_filter_item->cache_state(),
            RasterCacheItem::CacheState::kCurrent);
  EXPECT_EQ(cacheable_color_filter_item->GetId(),
            RasterCacheKeyID(layer->unique_id(), RasterCacheKeyType::kLayer));
  EXPECT_TRUE(raster_cache()->Draw(cacheable_color_filter_item->GetId().value(),
                                   cache_canvas, &paint));
  EXPECT_FALSE(raster_cache()->Draw(
      cacheable_color_filter_item->GetId().value(), other_canvas, &paint));
}

TEST_F(ColorFilterLayerTest, OpacityInheritance) {
  // clang-format off
  float matrix[20] = {
    0, 1, 0, 0, 0,
    0, 0, 1, 0, 0,
    1, 0, 0, 0, 0,
    0, 0, 0, 1, 0,
  };
  // clang-format on
  auto layer_filter = DlMatrixColorFilter(matrix);
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto color_filter_layer = std::make_shared<ColorFilterLayer>(
      std::make_shared<DlMatrixColorFilter>(matrix));
  color_filter_layer->Add(mock_layer);

  PrerollContext* context = preroll_context();
  context->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  color_filter_layer->Preroll(preroll_context());
  // ColorFilterLayer can always inherit opacity whether or not their
  // children are compatible.
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);

  int opacity_alpha = 0x7F;
  skity::Vec2 offset = skity::Vec2(10, 10);
  auto opacity_layer = std::make_shared<OpacityLayer>(opacity_alpha, offset);
  opacity_layer->Add(color_filter_layer);
  preroll_context()->state_stack.set_preroll_delegate(skity::Matrix());
  opacity_layer->Preroll(context);
  EXPECT_TRUE(opacity_layer->children_can_accept_opacity());

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    canvas->save();
    {
      canvas->translate(offset.x, offset.y);
      {
        SkPaint paint;
        paint.setColorFilter(layer_filter.gr_object());
        paint.setColor(opacity_alpha << 24);
        canvas->saveLayer(&child_path.getBounds(), &paint);
        {
          SkPaint inner_paint;
          inner_paint.setColor(0xFF000000);
          canvas->drawPath(child_path, inner_paint);
        }
        canvas->restore();
      }
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  opacity_layer->Paint(display_list_paint_context());
  EXPECT_TRUE(expected_display_list->approximateOpCount() ==
              display_list()->approximateOpCount());
}

}  // namespace testing
}  // namespace clay
