// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/layers/backdrop_filter_layer.h"
#include "clay/flow/layers/clip_rect_layer.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/flow/testing/diff_context_test.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/effects/SkImageFilters.h"

namespace clay {
namespace testing {

using BackdropFilterLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(BackdropFilterLayerTest, PaintingEmptyLayerDies) {
  auto filter = DlBlurImageFilter(5, 5, DlTileMode::kClamp);
  auto layer = std::make_shared<BackdropFilterLayer>(filter.shared(),
                                                     DlBlendMode::kSrcOver);
  auto parent = std::make_shared<ClipRectLayer>(kEmptyRect, Clip::hardEdge);
  parent->Add(layer);

  parent->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(BackdropFilterLayerTest, PaintBeforePrerollDies) {
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto filter = DlBlurImageFilter(5, 5, DlTileMode::kClamp);
  auto layer = std::make_shared<BackdropFilterLayer>(filter.shared(),
                                                     DlBlendMode::kSrcOver);
  layer->Add(mock_layer);

  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(BackdropFilterLayerTest, EmptyFilter) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer =
      std::make_shared<BackdropFilterLayer>(nullptr, DlBlendMode::kSrcOver);
  layer->Add(mock_layer);
  auto parent = std::make_shared<ClipRectLayer>(child_bounds, Clip::hardEdge);
  parent->Add(layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  parent->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0,
                       MockCanvas::SaveLayerData{
                           clay::ConvertSkityRectToSkRect(child_bounds),
                           SkPaint(), nullptr, 1}},
                   MockCanvas::DrawCall{
                       1, MockCanvas::DrawPathData{child_path, child_paint}},
                   MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(BackdropFilterLayerTest, SimpleFilter) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto layer_filter = SkImageFilters::Shader(
      SkShaders::Color(SkColors::kMagenta, /*colorSpace=*/nullptr));
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<BackdropFilterLayer>(
      std::make_shared<UnknownImageFilter>(layer_filter),
      DlBlendMode::kSrcOver);
  layer->Add(mock_layer);
  auto parent = std::make_shared<ClipRectLayer>(child_bounds, Clip::hardEdge);
  parent->Add(layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  parent->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0,
                       MockCanvas::SaveLayerData{
                           clay::ConvertSkityRectToSkRect(child_bounds),
                           SkPaint(), layer_filter, 1}},
                   MockCanvas::DrawCall{
                       1, MockCanvas::DrawPathData{child_path, child_paint}},
                   MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(BackdropFilterLayerTest, NonSrcOverBlend) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto layer_filter = SkImageFilters::Shader(
      SkShaders::Color(SkColors::kMagenta, /*colorSpace=*/nullptr));
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<BackdropFilterLayer>(
      std::make_shared<UnknownImageFilter>(layer_filter), DlBlendMode::kSrc);
  layer->Add(mock_layer);
  auto parent = std::make_shared<ClipRectLayer>(child_bounds, Clip::hardEdge);
  parent->Add(layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  parent->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_matrix(), initial_transform);

  SkPaint filter_paint = SkPaint();
  filter_paint.setBlendMode(SkBlendMode::kSrc);

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0,
                       MockCanvas::SaveLayerData{
                           clay::ConvertSkityRectToSkRect(child_bounds),
                           filter_paint, layer_filter, 1}},
                   MockCanvas::DrawCall{
                       1, MockCanvas::DrawPathData{child_path, child_paint}},
                   MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(BackdropFilterLayerTest, MultipleChildren) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 2.5f, 3.5f);
  const SkPath child_path1 =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath child_path2 = SkPath().addRect(
      clay::ConvertSkityRectToSkRect(child_bounds.MakeOffset(3.0f, 0.0f)));
  const SkPaint child_paint1 = SkPaint(SkColors::kYellow);
  const SkPaint child_paint2 = SkPaint(SkColors::kCyan);
  skity::Rect children_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  children_bounds.Join(clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  auto layer_filter = SkImageFilters::Shader(
      SkShaders::Color(SkColors::kMagenta, /*colorSpace=*/nullptr));
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer = std::make_shared<BackdropFilterLayer>(
      std::make_shared<UnknownImageFilter>(layer_filter),
      DlBlendMode::kSrcOver);
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);
  auto parent =
      std::make_shared<ClipRectLayer>(children_bounds, Clip::hardEdge);
  parent->Add(layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  parent->Preroll(preroll_context());
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

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0,
                       MockCanvas::SaveLayerData{
                           clay::ConvertSkityRectToSkRect(children_bounds),
                           SkPaint(), layer_filter, 1}},
                   MockCanvas::DrawCall{
                       1, MockCanvas::DrawPathData{child_path1, child_paint1}},
                   MockCanvas::DrawCall{
                       1, MockCanvas::DrawPathData{child_path2, child_paint2}},
                   MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(BackdropFilterLayerTest, Nested) {
  const skity::Matrix initial_transform = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 2.5f, 3.5f);
  const SkPath child_path1 =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath child_path2 = SkPath().addRect(
      clay::ConvertSkityRectToSkRect(child_bounds.MakeOffset(3.0f, 0.0f)));
  const SkPaint child_paint1 = SkPaint(SkColors::kYellow);
  const SkPaint child_paint2 = SkPaint(SkColors::kCyan);
  skity::Rect children_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  children_bounds.Join(clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  auto layer_filter1 = SkImageFilters::Shader(
      SkShaders::Color(SkColors::kMagenta, /*colorSpace=*/nullptr));
  auto layer_filter2 = SkImageFilters::Shader(
      SkShaders::Color(SkColors::kDkGray, /*colorSpace=*/nullptr));
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer1 = std::make_shared<BackdropFilterLayer>(
      std::make_shared<UnknownImageFilter>(layer_filter1),
      DlBlendMode::kSrcOver);
  auto layer2 = std::make_shared<BackdropFilterLayer>(
      std::make_shared<UnknownImageFilter>(layer_filter2),
      DlBlendMode::kSrcOver);
  layer2->Add(mock_layer2);
  layer1->Add(mock_layer1);
  layer1->Add(layer2);
  auto parent =
      std::make_shared<ClipRectLayer>(children_bounds, Clip::hardEdge);
  parent->Add(layer1);

  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  parent->Preroll(preroll_context());

  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer1->paint_bounds(), children_bounds);
  EXPECT_EQ(layer2->paint_bounds(), children_bounds);
  EXPECT_TRUE(mock_layer1->needs_painting(paint_context()));
  EXPECT_TRUE(mock_layer2->needs_painting(paint_context()));
  EXPECT_TRUE(layer1->needs_painting(paint_context()));
  EXPECT_TRUE(layer2->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer1->parent_matrix(), initial_transform);
  EXPECT_EQ(mock_layer2->parent_matrix(), initial_transform);

  layer1->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{
               0, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                children_bounds),
                                            SkPaint(), layer_filter1, 1}},
           MockCanvas::DrawCall{
               1, MockCanvas::DrawPathData{child_path1, child_paint1}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                children_bounds),
                                            SkPaint(), layer_filter2, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path2, child_paint2}},
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(BackdropFilterLayerTest, Readback) {
  std::shared_ptr<DlImageFilter> no_filter;
  auto layer_filter = DlBlurImageFilter(5, 5, DlTileMode::kClamp);
  auto initial_transform = skity::Matrix();

  // BDF with filter always reads from surface
  auto layer1 = std::make_shared<BackdropFilterLayer>(layer_filter.shared(),
                                                      DlBlendMode::kSrcOver);
  preroll_context()->surface_needs_readback = false;
  preroll_context()->state_stack.set_preroll_delegate(initial_transform);
  layer1->Preroll(preroll_context());
  EXPECT_TRUE(preroll_context()->surface_needs_readback);

  // BDF with no filter does not read from surface itself
  auto layer2 =
      std::make_shared<BackdropFilterLayer>(no_filter, DlBlendMode::kSrcOver);
  preroll_context()->surface_needs_readback = false;
  layer2->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);

  // BDF with no filter does not block prior readback value
  preroll_context()->surface_needs_readback = true;
  layer2->Preroll(preroll_context());
  EXPECT_TRUE(preroll_context()->surface_needs_readback);

  // BDF with no filter blocks child with readback
  auto mock_layer = std::make_shared<MockLayer>(SkPath(), SkPaint());
  mock_layer->set_fake_reads_surface(true);
  layer2->Add(mock_layer);
  preroll_context()->surface_needs_readback = false;
  layer2->Preroll(preroll_context());
  EXPECT_FALSE(preroll_context()->surface_needs_readback);
}

TEST_F(BackdropFilterLayerTest, OpacityInheritance) {
  auto backdrop_filter = DlBlurImageFilter(5, 5, DlTileMode::kClamp);
  const SkPath mock_path = SkPath().addRect(SkRect::MakeLTRB(0, 0, 10, 10));
  const SkPaint mock_paint = SkPaint(SkColors::kRed);
  const SkRect clip_rect = SkRect::MakeLTRB(0, 0, 100, 100);

  auto clip = std::make_shared<ClipRectLayer>(
      clay::ConvertSkRectToSkityRect(clip_rect), Clip::hardEdge);
  auto parent = std::make_shared<OpacityLayer>(128, skity::Vec2(0, 0));
  auto layer = std::make_shared<BackdropFilterLayer>(backdrop_filter.shared(),
                                                     DlBlendMode::kSrcOver);
  auto child = std::make_shared<MockLayer>(mock_path, mock_paint);
  layer->Add(child);
  parent->Add(layer);
  clip->Add(parent);

  clip->Preroll(preroll_context());

  clip->Paint(display_list_paint_context());

  SkPictureRecorder recorder;
  auto rtree = std::make_unique<SkRTreeFactory>();
  auto canvas = recorder.beginRecording(kDlBounds, rtree.get());
  {
    canvas->save();
    {
      canvas->clipRect(clip_rect, SkClipOp::kIntersect, false);
      {
        SkPaint save_paint;
        save_paint.setAlpha(128);
        save_paint.setImageFilter(backdrop_filter.gr_object());
        canvas->saveLayer(clip_rect, &save_paint);
        {
          SkPaint child_paint;
          child_paint.setColor(SK_ColorRED);
          canvas->drawPath(mock_path, child_paint);
        }
        canvas->restore();
      }
    }
    canvas->restore();
  }
  auto expected_display_list = recorder.finishRecordingAsPicture();

  EXPECT_TRUE(expected_display_list->approximateOpCount() ==
              display_list()->approximateOpCount());
}

using BackdropLayerDiffTest = DiffContextTest;

TEST_F(BackdropLayerDiffTest, BackdropLayer) {
  auto filter = DlBlurImageFilter(10, 10, DlTileMode::kClamp);

  {
    // tests later assume 30px readback area, fail early if that's not the case
    skity::Rect readback;
    EXPECT_EQ(filter.get_input_device_bounds(skity::Rect::MakeWH(10, 10),
                                             skity::Matrix(), readback),
              &readback);
    EXPECT_EQ(readback, skity::Rect::MakeLTRB(-30, -30, 40, 40));
  }

  MockLayerTree l1({100, 100});
  l1.root()->Add(std::make_shared<BackdropFilterLayer>(filter.shared(),
                                                       DlBlendMode::kSrcOver));

  // no clip, effect over entire surface
  auto damage = DiffLayerTree(l1, MockLayerTree({100, 100}));
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeWH(100, 100));

  MockLayerTree l2({100, 100});

  auto clip = std::make_shared<ClipRectLayer>(
      skity::Rect::MakeLTRB(20, 20, 60, 60), Clip::hardEdge);
  clip->Add(std::make_shared<BackdropFilterLayer>(filter.shared(),
                                                  DlBlendMode::kSrcOver));
  l2.root()->Add(clip);
  damage = DiffLayerTree(l2, MockLayerTree({100, 100}));

  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 90, 90));

  MockLayerTree l3;
  auto scale = std::make_shared<TransformLayer>(skity::Matrix::Scale(2.0, 2.0));
  scale->Add(clip);
  l3.root()->Add(scale);

  damage = DiffLayerTree(l3, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 180, 180));

  MockLayerTree l4;
  l4.root()->Add(scale);

  // path just outside of readback region, doesn't affect blur
  auto path1 = SkPath().addRect(SkRect::MakeLTRB(180, 180, 190, 190));
  l4.root()->Add(std::make_shared<MockLayer>(path1));
  damage = DiffLayerTree(l4, l3);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(180, 180, 190, 190));

  MockLayerTree l5;
  l5.root()->Add(scale);

  // path just inside of readback region, must trigger backdrop repaint
  auto path2 = SkPath().addRect(SkRect::MakeLTRB(179, 179, 189, 189));
  l5.root()->Add(std::make_shared<MockLayer>(path2));
  damage = DiffLayerTree(l5, l4);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 190, 190));
}

TEST_F(BackdropLayerDiffTest, BackdropLayerInvalidTransform) {
  auto filter = DlBlurImageFilter(10, 10, DlTileMode::kClamp);

  {
    // tests later assume 30px readback area, fail early if that's not the case
    skity::Rect readback;
    EXPECT_EQ(filter.get_input_device_bounds(skity::Rect::MakeWH(10, 10),
                                             skity::Matrix(), readback),
              &readback);
    EXPECT_EQ(readback, skity::Rect::MakeLTRB(-30, -30, 40, 40));
  }

  MockLayerTree l1({100, 100});
  skity::Matrix transform;
  transform.SetPersp0(0.1);
  transform.SetPersp1(0.1);

  auto transform_layer = std::make_shared<TransformLayer>(transform);
  l1.root()->Add(transform_layer);
  transform_layer->Add(std::make_shared<BackdropFilterLayer>(
      filter.shared(), DlBlendMode::kSrcOver));

  auto damage = DiffLayerTree(l1, MockLayerTree({100, 100}));
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeWH(100, 100));
}

}  // namespace testing
}  // namespace clay
