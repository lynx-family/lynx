// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/layers/image_filter_layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/flow/testing/diff_context_test.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/gfx/testing_utils.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/effects/SkImageFilters.h"
namespace clay {
namespace testing {

using ImageFilterLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(ImageFilterLayerTest, PaintingEmptyLayerDies) {
  auto layer = std::make_shared<ImageFilterLayer>(nullptr);

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(ImageFilterLayerTest, PaintBeforePrerollDies) {
  const SkRect child_bounds = SkRect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path = SkPath().addRect(child_bounds);
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ImageFilterLayer>(nullptr);
  layer->Add(mock_layer);

  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(ImageFilterLayerTest, EmptyFilter) {
  const SkMatrix initial_transform = SkMatrix::Translate(0.5f, 1.0f);
  const SkRect child_bounds = SkRect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path = SkPath().addRect(child_bounds);
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ImageFilterLayer>(nullptr);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_bounds));
  EXPECT_EQ(layer->child_paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_bounds));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(),
            clay::ConvertSkMatrixToSkityMatrix(initial_transform));

  SkPaint filter_paint;
  filter_paint.setImageFilter(nullptr);
  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({
                MockCanvas::DrawCall{
                    0, MockCanvas::DrawPathData{child_path, child_paint}},
            }));
}

TEST_F(ImageFilterLayerTest, SimpleFilter) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ImageFilterLayer>(dl_image_filter);
  layer->Add(mock_layer);

  const skity::Rect child_rounded_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 21.0f, 22.0f);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), child_rounded_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    SkPaint paint;
    paint.setImageFilter(dl_image_filter->gr_object());
    auto bounds_tmp = clay::ConvertSkityRectToSkRect(child_bounds);
    canvas->saveLayer(&bounds_tmp, &paint);
    {
      SkPaint new_paint;
      new_paint.setColor(SK_ColorYELLOW);
      canvas->drawPath(child_path, new_paint);
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  layer->Paint(display_list_paint_context());
  EXPECT_TRUE(display_list()->approximateOpCount() ==
              expected_display_list->approximateOpCount());
}

TEST_F(ImageFilterLayerTest, SimpleFilterWithOffset) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect initial_cull_rect = skity::Rect::MakeLTRB(0, 0, 100, 100);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  const skity::Vec2 layer_offset = skity::Vec2(5.5, 6.5);
  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer =
      std::make_shared<ImageFilterLayer>(dl_image_filter, layer_offset);
  layer->Add(mock_layer);

  skity::Matrix child_matrix = initial_transform;
  child_matrix.PreTranslate(layer_offset.x, layer_offset.y);
  const skity::Rect child_rounded_bounds =
      skity::Rect::MakeLTRB(10.5f, 12.5f, 26.5f, 28.5f);

  preroll_context()->state_stack.set_preroll_delegate(initial_cull_rect,
                                                      initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), child_rounded_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), child_matrix);
  EXPECT_EQ(preroll_context()->state_stack.device_cull_rect(),
            initial_cull_rect);

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    canvas->save();
    {
      canvas->translate(layer_offset.x, layer_offset.y);
      SkPaint sk_paint;
      sk_paint.setImageFilter(dl_image_filter->gr_object());
      auto bounds_tmp = clay::ConvertSkityRectToSkRect(child_bounds);
      canvas->saveLayer(&bounds_tmp, &sk_paint);
      {
        SkPaint new_paint;
        new_paint.setColor(SK_ColorYELLOW);
        canvas->drawPath(child_path, new_paint);
      }
      canvas->restore();
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  layer->Paint(display_list_paint_context());
  EXPECT_TRUE(display_list()->approximateOpCount() ==
              expected_display_list->approximateOpCount());
}

TEST_F(ImageFilterLayerTest, SimpleFilterBounds) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  const skity::Matrix filter_transform = skity::Matrix::Scale(2.0, 2.0);

  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      filter_transform, DlImageSampling::kMipmapLinear);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ImageFilterLayer>(dl_image_filter);
  layer->Add(mock_layer);

  const skity::Rect filter_bounds =
      skity::Rect::MakeLTRB(10.0f, 12.0f, 42.0f, 44.0f);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), filter_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    SkPaint sk_paint;
    sk_paint.setImageFilter(dl_image_filter->gr_object());
    auto bounds_tmp = clay::ConvertSkityRectToSkRect(child_bounds);
    canvas->saveLayer(&bounds_tmp, &sk_paint);
    {
      SkPaint new_paint;
      new_paint.setColor(SK_ColorYELLOW);
      canvas->drawPath(child_path, new_paint);
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  layer->Paint(display_list_paint_context());
  EXPECT_TRUE(display_list()->approximateOpCount() ==
              expected_display_list->approximateOpCount());
}

TEST_F(ImageFilterLayerTest, MultipleChildren) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 2.5f, 3.5f);
  const SkPath child_path1 =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath child_path2 = SkPath().addRect(
      clay::ConvertSkityRectToSkRect(child_bounds.MakeOffset(3.0f, 0.0f)));
  const SkPaint child_paint1 = SkPaint(SkColors::kYellow);
  const SkPaint child_paint2 = SkPaint(SkColors::kCyan);
  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer = std::make_shared<ImageFilterLayer>(dl_image_filter);
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  skity::Rect children_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  children_bounds.Join(clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  skity::Rect children_rounded_bounds = children_bounds;
  children_rounded_bounds.RoundOut();

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), children_rounded_bounds);
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
    SkPaint sk_paint;
    sk_paint.setImageFilter(dl_image_filter->gr_object());
    auto bounds_tmp = clay::ConvertSkityRectToSkRect(children_bounds);
    canvas->saveLayer(&bounds_tmp, &sk_paint);
    {
      SkPaint new_paint;
      new_paint.setColor(SK_ColorYELLOW);
      canvas->drawPath(child_path1, new_paint);
    }
    {
      SkPaint new_paint;
      new_paint.setColor(SK_ColorCYAN);
      canvas->drawPath(child_path2, new_paint);
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  layer->Paint(display_list_paint_context());
  EXPECT_TRUE(display_list()->approximateOpCount() ==
              expected_display_list->approximateOpCount());
}

TEST_F(ImageFilterLayerTest, Nested) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 2.5f, 3.5f);
  const SkPath child_path1 =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath child_path2 = SkPath().addRect(
      clay::ConvertSkityRectToSkRect(child_bounds.MakeOffset(3.0f, 0.0f)));
  const SkPaint child_paint1 = SkPaint(SkColors::kYellow);
  const SkPaint child_paint2 = SkPaint(SkColors::kCyan);
  auto dl_image_filter1 = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);
  auto dl_image_filter2 = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer1 = std::make_shared<ImageFilterLayer>(dl_image_filter1);
  auto layer2 = std::make_shared<ImageFilterLayer>(dl_image_filter2);
  layer2->Add(mock_layer2);
  layer1->Add(mock_layer1);
  layer1->Add(layer2);

  skity::Rect children_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  skity::Rect child_bounds_tmp =
      clay::ConvertSkRectToSkityRect(child_path2.getBounds());
  child_bounds_tmp.RoundOut();
  children_bounds.Join(child_bounds_tmp);
  skity::Rect children_rounded_bounds = children_bounds;
  children_rounded_bounds.RoundOut();
  skity::Rect mock_layer2_rounded_bounds =
      clay::ConvertSkRectToSkityRect(child_path2.getBounds());
  mock_layer2_rounded_bounds.RoundOut();

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer1->Preroll(preroll_context());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer1->paint_bounds(), children_rounded_bounds);
  EXPECT_EQ(layer1->child_paint_bounds(), children_bounds);
  EXPECT_EQ(layer2->paint_bounds(), mock_layer2_rounded_bounds);
  EXPECT_EQ(layer2->child_paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
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
    SkPaint sk_paint;
    sk_paint.setImageFilter(dl_image_filter1->gr_object());
    auto bounds_tmp = clay::ConvertSkityRectToSkRect(children_bounds);
    canvas->saveLayer(&bounds_tmp, &sk_paint);
    {
      SkPaint new_paint;
      new_paint.setColor(SK_ColorYELLOW);
      canvas->drawPath(child_path1, new_paint);
    }
    {
      SkPaint child_paint;
      child_paint.setImageFilter(dl_image_filter2->gr_object());
      canvas->saveLayer(&child_path2.getBounds(), &child_paint);
      {
        SkPaint new_paint;
        new_paint.setColor(SK_ColorCYAN);
        canvas->drawPath(child_path2, new_paint);
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

TEST_F(ImageFilterLayerTest, Readback) {
  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kLinear);

  // ImageFilterLayer does not read from surface
  auto layer = std::make_shared<ImageFilterLayer>(dl_image_filter);
  preroll_context()->surface_needs_readback = false;
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);

  // ImageFilterLayer blocks child with readback
  auto mock_layer = std::make_shared<MockLayer>(SkPath(), SkPaint());
  mock_layer->set_fake_reads_surface(true);
  layer->Add(mock_layer);
  preroll_context()->surface_needs_readback = false;
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);
}

TEST_F(ImageFilterLayerTest, CacheChild) {
  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  auto other_transform = SkMatrix::Scale(1.0, 2.0);
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ImageFilterLayer>(dl_image_filter);
  layer->Add(mock_layer);

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);
  SkCanvas other_canvas;
  other_canvas.setMatrix(other_transform);
  SkPaint paint = SkPaint();

  use_mock_raster_cache();
  const auto* cacheable_image_filter_item = layer->raster_cache_item();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  // ImageFilterLayer default cache itself.
  EXPECT_EQ(cacheable_image_filter_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_image_filter_item->Draw(paint_context(), &paint));

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)1);
  // The layer_cache_item's strategy is Children, mean we will must cache
  // his children
  EXPECT_EQ(cacheable_image_filter_item->cache_state(),
            RasterCacheItem::CacheState::kChildren);
  EXPECT_TRUE(raster_cache()->Draw(cacheable_image_filter_item->GetId().value(),
                                   cache_canvas, &paint));
  EXPECT_FALSE(raster_cache()->Draw(
      cacheable_image_filter_item->GetId().value(), other_canvas, &paint));
}

TEST_F(ImageFilterLayerTest, CacheChildren) {
  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  auto other_transform = SkMatrix::Scale(1.0, 2.0);
  SkPaint paint = SkPaint();
  const SkPath child_path1 = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  const SkPath child_path2 = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2);
  auto layer = std::make_shared<ImageFilterLayer>(dl_image_filter);
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);
  SkCanvas other_canvas;
  other_canvas.setMatrix(other_transform);

  use_mock_raster_cache();

  const auto* cacheable_image_filter_item = layer->raster_cache_item();
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);

  // ImageFilterLayer default cache itself.
  EXPECT_EQ(cacheable_image_filter_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_image_filter_item->Draw(paint_context(), &paint));

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)1);

  // The layer_cache_item's strategy is Children, mean we will must cache his
  // children
  EXPECT_EQ(cacheable_image_filter_item->cache_state(),
            RasterCacheItem::CacheState::kChildren);
  EXPECT_TRUE(raster_cache()->Draw(cacheable_image_filter_item->GetId().value(),
                                   cache_canvas, &paint));
  EXPECT_FALSE(raster_cache()->Draw(
      cacheable_image_filter_item->GetId().value(), other_canvas, &paint));
}

TEST_F(ImageFilterLayerTest, CacheImageFilterLayerSelf) {
  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);

  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  auto other_transform = SkMatrix::Scale(1.0, 2.0);
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ImageFilterLayer>(dl_image_filter);
  layer->Add(mock_layer);

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);
  SkCanvas other_canvas;
  other_canvas.setMatrix(other_transform);
  SkPaint paint = SkPaint();

  use_mock_raster_cache();
  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  const auto* cacheable_image_filter_item = layer->raster_cache_item();
  // frame 1.
  layer->Preroll(preroll_context());
  layer->Paint(paint_context());
  // frame 2.
  layer->Preroll(preroll_context());
  layer->Paint(paint_context());
  // frame 3.
  layer->Preroll(preroll_context());
  layer->Paint(paint_context());

  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());
  // frame1,2 cache the ImageFilter's children layer, frame3 cache the
  // ImageFilterLayer
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)2);

  // ImageFilterLayer default cache itself.
  EXPECT_EQ(cacheable_image_filter_item->cache_state(),
            RasterCacheItem::CacheState::kCurrent);
  EXPECT_EQ(cacheable_image_filter_item->GetId(),
            RasterCacheKeyID(layer->unique_id(), RasterCacheKeyType::kLayer));
  EXPECT_TRUE(raster_cache()->Draw(cacheable_image_filter_item->GetId().value(),
                                   cache_canvas, &paint));
  EXPECT_FALSE(raster_cache()->Draw(
      cacheable_image_filter_item->GetId().value(), other_canvas, &paint));
}

TEST_F(ImageFilterLayerTest, OpacityInheritance) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto dl_image_filter = std::make_shared<DlMatrixImageFilter>(
      skity::Matrix(), DlImageSampling::kMipmapLinear);

  // The mock_layer child will not be compatible with opacity
  auto mock_layer = MockLayer::Make(child_path, child_paint);
  auto image_filter_layer = std::make_shared<ImageFilterLayer>(dl_image_filter);
  image_filter_layer->Add(mock_layer);

  PrerollContext* context = preroll_context();
  context->state_stack.set_preroll_delegate(initial_transform);
  image_filter_layer->Preroll(preroll_context());
  // ImageFilterLayers can always inherit opacity whether or not their
  // children are compatible.
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity |
                LayerStateStack::kCallerCanApplyColorFilter);

  int opacity_alpha = 0x7F;
  skity::Vec2 offset = skity::Vec2(10, 10);
  auto opacity_layer = std::make_shared<OpacityLayer>(opacity_alpha, offset);
  opacity_layer->Add(image_filter_layer);
  context->state_stack.set_preroll_delegate(skity::Matrix());
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
        SkPaint sk_paint;
        sk_paint.setColor(opacity_alpha << 24);
        sk_paint.setImageFilter(dl_image_filter->gr_object());
        canvas->saveLayer(&child_path.getBounds(), &sk_paint);
        {
          SkPaint new_paint;
          new_paint.setColor(child_paint.getColor());
          canvas->drawPath(child_path, new_paint);
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

using ImageFilterLayerDiffTest = DiffContextTest;

TEST_F(ImageFilterLayerDiffTest, ImageFilterLayer) {
  auto dl_blur_filter =
      std::make_shared<DlBlurImageFilter>(10, 10, DlTileMode::kClamp);
  {
    // tests later assume 30px paint area, fail early if that's not the case
    skity::Rect input_bounds;
    dl_blur_filter->get_input_device_bounds(skity::Rect::MakeWH(10, 10),
                                            skity::Matrix(), input_bounds);
    EXPECT_EQ(input_bounds, skity::Rect::MakeLTRB(-30, -30, 40, 40));
  }

  MockLayerTree l1;
  auto filter_layer = std::make_shared<ImageFilterLayer>(dl_blur_filter);
  auto path = SkPath().addRect(SkRect::MakeLTRB(100, 100, 110, 110));
  filter_layer->Add(std::make_shared<MockLayer>(path));
  l1.root()->Add(filter_layer);

  auto damage = DiffLayerTree(l1, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(70, 70, 140, 140));

  MockLayerTree l2;
  auto scale = std::make_shared<TransformLayer>(skity::Matrix::Scale(2.0, 2.0));
  scale->Add(filter_layer);
  l2.root()->Add(scale);

  damage = DiffLayerTree(l2, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(140, 140, 280, 280));

  MockLayerTree l3;
  l3.root()->Add(scale);

  // path outside of ImageFilterLayer
  auto path1 = SkPath().addRect(SkRect::MakeLTRB(130, 130, 140, 140));
  l3.root()->Add(std::make_shared<MockLayer>(path1));
  damage = DiffLayerTree(l3, l2);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(130, 130, 140, 140));

  // path intersecting ImageFilterLayer, shouldn't trigger entire
  // ImageFilterLayer repaint
  MockLayerTree l4;
  l4.root()->Add(scale);
  auto path2 = SkPath().addRect(SkRect::MakeLTRB(130, 130, 141, 141));
  l4.root()->Add(std::make_shared<MockLayer>(path2));
  damage = DiffLayerTree(l4, l3);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(130, 130, 141, 141));
}

TEST_F(ImageFilterLayerDiffTest, ImageFilterLayerInflatestChildSize) {
  auto dl_blur_filter =
      std::make_shared<DlBlurImageFilter>(10, 10, DlTileMode::kClamp);

  {
    // tests later assume 30px paint area, fail early if that's not the case
    skity::Rect input_bounds;
    dl_blur_filter->get_input_device_bounds(skity::Rect::MakeWH(10, 10),
                                            skity::Matrix(), input_bounds);
    EXPECT_EQ(input_bounds, skity::Rect::MakeLTRB(-30, -30, 40, 40));
  }

  MockLayerTree l1;

  // Use nested filter layers to check if both contribute to child bounds
  auto filter_layer_1_1 = std::make_shared<ImageFilterLayer>(dl_blur_filter);
  auto filter_layer_1_2 = std::make_shared<ImageFilterLayer>(dl_blur_filter);
  filter_layer_1_1->Add(filter_layer_1_2);
  auto path = SkPath().addRect(SkRect::MakeLTRB(100, 100, 110, 110));
  filter_layer_1_2->Add(
      std::make_shared<MockLayer>(path, SkPaint(SkColors::kYellow)));
  l1.root()->Add(filter_layer_1_1);

  // second layer tree with identical filter layers but different child layer
  MockLayerTree l2;
  auto filter_layer2_1 = std::make_shared<ImageFilterLayer>(dl_blur_filter);
  filter_layer2_1->AssignOldLayer(filter_layer_1_1.get());
  auto filter_layer2_2 = std::make_shared<ImageFilterLayer>(dl_blur_filter);
  filter_layer2_2->AssignOldLayer(filter_layer_1_2.get());
  filter_layer2_1->Add(filter_layer2_2);
  filter_layer2_2->Add(
      std::make_shared<MockLayer>(path, SkPaint(SkColors::kRed)));
  l2.root()->Add(filter_layer2_1);

  DiffLayerTree(l1, MockLayerTree());
  auto damage = DiffLayerTree(l2, l1);

  // ensure that filter properly inflated child size
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(40, 40, 170, 170));
}

}  // namespace testing
}  // namespace clay
