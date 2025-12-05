// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/layers/shader_mask_layer.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkShader.h"
#include "third_party/skia/include/effects/SkPerlinNoiseShader.h"

namespace clay {
namespace testing {

using ShaderMaskLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(ShaderMaskLayerTest, PaintingEmptyLayerDies) {
  auto layer =
      std::make_shared<ShaderMaskLayer>(nullptr, kEmptyRect, DlBlendMode::kSrc);

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(ShaderMaskLayerTest, PaintBeforePrerollDies) {
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer =
      std::make_shared<ShaderMaskLayer>(nullptr, kEmptyRect, DlBlendMode::kSrc);
  layer->Add(mock_layer);

  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(ShaderMaskLayerTest, EmptyFilter) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const skity::Rect layer_bounds =
      skity::Rect::MakeLTRB(2.0f, 4.0f, 6.5f, 6.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ShaderMaskLayer>(nullptr, layer_bounds,
                                                 DlBlendMode::kSrc);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  SkPaint filter_paint;
  filter_paint.setBlendMode(SkBlendMode::kSrc);
  filter_paint.setShader(nullptr);
  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{
               0, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            SkPaint(), nullptr, 1}},
           MockCanvas::DrawCall{
               1, MockCanvas::DrawPathData{child_path, child_paint}},
           MockCanvas::DrawCall{1,
                                MockCanvas::ConcatMatrixData{SkM44::Translate(
                                    layer_bounds.Left(), layer_bounds.Top())}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::DrawRectData{
                   SkRect::MakeWH(layer_bounds.Width(), layer_bounds.Height()),
                   filter_paint}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(ShaderMaskLayerTest, SimpleFilter) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const skity::Rect layer_bounds =
      skity::Rect::MakeLTRB(2.0f, 4.0f, 6.5f, 6.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto layer_filter =
      SkPerlinNoiseShader::MakeFractalNoise(1.0f, 1.0f, 1, 1.0f);
  auto dl_filter = std::make_shared<UnknownColorSource>(layer_filter);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ShaderMaskLayer>(dl_filter, layer_bounds,
                                                 DlBlendMode::kSrc);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  SkPaint filter_paint;
  filter_paint.setBlendMode(SkBlendMode::kSrc);
  filter_paint.setShader(layer_filter);
  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{
               0, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            SkPaint(), nullptr, 1}},
           MockCanvas::DrawCall{
               1, MockCanvas::DrawPathData{child_path, child_paint}},
           MockCanvas::DrawCall{1,
                                MockCanvas::ConcatMatrixData{SkM44::Translate(
                                    layer_bounds.Left(), layer_bounds.Top())}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::DrawRectData{
                   SkRect::MakeWH(layer_bounds.Width(), layer_bounds.Height()),
                   filter_paint}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(ShaderMaskLayerTest, MultipleChildren) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const skity::Rect layer_bounds =
      skity::Rect::MakeLTRB(2.0f, 4.0f, 6.5f, 6.5f);
  const SkPath child_path1 =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath child_path2 = SkPath().addRect(
      clay::ConvertSkityRectToSkRect(child_bounds.MakeOffset(3.0f, 0.0f)));
  const SkPaint child_paint1 = SkPaint(SkColors::kYellow);
  const SkPaint child_paint2 = SkPaint(SkColors::kCyan);
  auto layer_filter =
      SkPerlinNoiseShader::MakeFractalNoise(1.0f, 1.0f, 1, 1.0f);
  auto dl_filter = std::make_shared<UnknownColorSource>(layer_filter);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer = std::make_shared<ShaderMaskLayer>(dl_filter, layer_bounds,
                                                 DlBlendMode::kSrc);
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

  SkPaint filter_paint;
  filter_paint.setBlendMode(SkBlendMode::kSrc);
  filter_paint.setShader(layer_filter);
  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{
               0, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                children_bounds),
                                            SkPaint(), nullptr, 1}},
           MockCanvas::DrawCall{
               1, MockCanvas::DrawPathData{child_path1, child_paint1}},
           MockCanvas::DrawCall{
               1, MockCanvas::DrawPathData{child_path2, child_paint2}},
           MockCanvas::DrawCall{1,
                                MockCanvas::ConcatMatrixData{SkM44::Translate(
                                    layer_bounds.Left(), layer_bounds.Top())}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::DrawRectData{
                   SkRect::MakeWH(layer_bounds.Width(), layer_bounds.Height()),
                   filter_paint}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(ShaderMaskLayerTest, Nested) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 7.5f, 8.5f);
  const skity::Rect layer_bounds =
      skity::Rect::MakeLTRB(2.0f, 4.0f, 20.5f, 20.5f);
  const SkPath child_path1 =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath child_path2 = SkPath().addRect(
      clay::ConvertSkityRectToSkRect(child_bounds.MakeOffset(3.0f, 0.0f)));
  const SkPaint child_paint1 = SkPaint(SkColors::kYellow);
  const SkPaint child_paint2 = SkPaint(SkColors::kCyan);
  auto layer_filter1 =
      SkPerlinNoiseShader::MakeFractalNoise(1.0f, 1.0f, 1, 1.0f);
  auto dl_filter1 = std::make_shared<UnknownColorSource>(layer_filter1);
  auto layer_filter2 =
      SkPerlinNoiseShader::MakeFractalNoise(2.0f, 2.0f, 2, 2.0f);
  auto dl_filter2 = std::make_shared<UnknownColorSource>(layer_filter2);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer1 = std::make_shared<ShaderMaskLayer>(dl_filter1, layer_bounds,
                                                  DlBlendMode::kSrc);
  auto layer2 = std::make_shared<ShaderMaskLayer>(dl_filter2, layer_bounds,
                                                  DlBlendMode::kSrc);
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

  SkPaint filter_paint1, filter_paint2;
  filter_paint1.setBlendMode(SkBlendMode::kSrc);
  filter_paint2.setBlendMode(SkBlendMode::kSrc);
  filter_paint1.setShader(layer_filter1);
  filter_paint2.setShader(layer_filter2);
  layer1->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{
               0, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                children_bounds),
                                            SkPaint(), nullptr, 1}},
           MockCanvas::DrawCall{
               1, MockCanvas::DrawPathData{child_path1, child_paint1}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{child_path2.getBounds(), SkPaint(),
                                            nullptr, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path2, child_paint2}},
           MockCanvas::DrawCall{2,
                                MockCanvas::ConcatMatrixData{SkM44::Translate(
                                    layer_bounds.Left(), layer_bounds.Top())}},
           MockCanvas::DrawCall{
               2,
               MockCanvas::DrawRectData{
                   SkRect::MakeWH(layer_bounds.Width(), layer_bounds.Height()),
                   filter_paint2}},
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1,
                                MockCanvas::ConcatMatrixData{SkM44::Translate(
                                    layer_bounds.Left(), layer_bounds.Top())}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::DrawRectData{
                   SkRect::MakeWH(layer_bounds.Width(), layer_bounds.Height()),
                   filter_paint1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(ShaderMaskLayerTest, Readback) {
  const skity::Rect layer_bounds =
      skity::Rect::MakeLTRB(2.0f, 4.0f, 20.5f, 20.5f);
  auto layer_filter =
      SkPerlinNoiseShader::MakeFractalNoise(1.0f, 1.0f, 1, 1.0f);
  auto dl_filter = std::make_shared<UnknownColorSource>(layer_filter);
  auto layer = std::make_shared<ShaderMaskLayer>(dl_filter, layer_bounds,
                                                 DlBlendMode::kSrc);

  // ShaderMaskLayer does not read from surface
  preroll_context()->surface_needs_readback = false;
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);

  // ShaderMaskLayer blocks child with readback
  auto mock_layer = std::make_shared<MockLayer>(SkPath(), SkPaint());
  mock_layer->set_fake_reads_surface(true);
  layer->Add(mock_layer);
  preroll_context()->surface_needs_readback = false;
  layer->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);
}

TEST_F(ShaderMaskLayerTest, LayerCached) {
  auto layer_filter =
      SkPerlinNoiseShader::MakeFractalNoise(1.0f, 1.0f, 1, 1.0f);
  auto dl_filter = std::make_shared<UnknownColorSource>(layer_filter);
  SkPaint paint;
  const SkRect layer_bounds = SkRect::MakeLTRB(2.0f, 4.0f, 20.5f, 20.5f);
  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ShaderMaskLayer>(
      dl_filter, clay::ConvertSkRectToSkityRect(layer_bounds),
      DlBlendMode::kSrc);
  layer->Add(mock_layer);

  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);

  use_mock_raster_cache();
  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  const auto* cacheable_shader_masker_item = layer->raster_cache_item();

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(cacheable_shader_masker_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_shader_masker_item->GetId().has_value());

  // frame 1.
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(cacheable_shader_masker_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_shader_masker_item->GetId().has_value());

  // frame 2.
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(cacheable_shader_masker_item->cache_state(),
            RasterCacheItem::CacheState::kNone);
  EXPECT_FALSE(cacheable_shader_masker_item->GetId().has_value());

  // frame 3.
  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)1);
  EXPECT_EQ(cacheable_shader_masker_item->cache_state(),
            RasterCacheItem::CacheState::kCurrent);

  EXPECT_TRUE(raster_cache()->Draw(
      cacheable_shader_masker_item->GetId().value(), cache_canvas, &paint));
}

TEST_F(ShaderMaskLayerTest, OpacityInheritance) {
  const SkRect child_bounds = SkRect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path = SkPath().addRect(child_bounds);
  auto mock_layer = MockLayer::Make(child_path);
  const skity::Rect mask_rect = skity::Rect::MakeLTRB(10, 10, 20, 20);
  auto shader_mask_layer =
      std::make_shared<ShaderMaskLayer>(nullptr, mask_rect, DlBlendMode::kSrc);
  shader_mask_layer->Add(mock_layer);

  // ShaderMaskLayers can always support opacity despite incompatible children
  PrerollContext* context = preroll_context();
  shader_mask_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags, Layer::kSaveLayerRenderFlags);

  int opacity_alpha = 0x7F;
  skity::Vec2 offset = skity::Vec2(10, 10);
  auto opacity_layer = std::make_shared<OpacityLayer>(opacity_alpha, offset);
  opacity_layer->Add(shader_mask_layer);
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
        canvas->saveLayer(&child_path.getBounds(), &sk_paint);
        {
          { canvas->drawPath(child_path, SkPaint()); }
          canvas->translate(mask_rect.Left(), mask_rect.Top());
          SkPaint new_paint;
          new_paint.setBlendMode(SkBlendMode::kSrc);
          canvas->drawRect(
              SkRect::MakeWH(mask_rect.Width(), mask_rect.Height()), new_paint);
        }
        canvas->restore();
      }
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  opacity_layer->Paint(display_list_paint_context());
  EXPECT_TRUE((expected_display_list->approximateOpCount() ==
               display_list()->approximateOpCount()));
}

#ifndef SUPPORT_FRACTIONAL_TRANSLATION
TEST_F(ShaderMaskLayerTest, SimpleFilterWithRasterCache) {
  use_mock_raster_cache();  // Ensure non-fractional alignment.

  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const skity::Rect layer_bounds =
      skity::Rect::MakeLTRB(2.0f, 4.0f, 6.5f, 6.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto layer_filter =
      SkPerlinNoiseShader::MakeFractalNoise(1.0f, 1.0f, 1, 1.0f);
  auto dl_filter = DlColorSource::From(layer_filter);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ShaderMaskLayer>(dl_filter, layer_bounds,
                                                 DlBlendMode::kSrc);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));
  layer->Preroll(preroll_context());

  SkPaint filter_paint;
  filter_paint.setBlendMode(SkBlendMode::kSrc);
  filter_paint.setShader(layer_filter);
  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
           MockCanvas::DrawCall{1, MockCanvas::SetMatrixData{skity::Matrix(
                                       skity::Matrix::Translate(0.0, 0.0))}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            SkPaint(), nullptr, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path, child_paint}},
           MockCanvas::DrawCall{
               2, MockCanvas::ConcatMatrixData{skity::Matrix::Translate(
                      layer_bounds.fLeft, layer_bounds.fTop)}},
           MockCanvas::DrawCall{2,
                                MockCanvas::DrawRectData{
                                    skity::Rect::MakeWH(layer_bounds.width(),
                                                        layer_bounds.height()),
                                    filter_paint}},
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}
#endif

}  // namespace testing
}  // namespace clay
