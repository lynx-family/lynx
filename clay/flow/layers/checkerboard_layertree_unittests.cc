// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/layers/clip_path_layer.h"
#include "clay/flow/layers/clip_rect_layer.h"
#include "clay/flow/layers/clip_rrect_layer.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/mock_canvas.h"

namespace clay {
namespace testing {

using CheckerBoardLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(CheckerBoardLayerTest, ClipRectSaveLayerCheckBoard) {
  const skity::Matrix initial_matrix = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds = skity::Rect::MakeXYWH(1.0, 2.0, 2.0, 2.0);
  const skity::Rect layer_bounds = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ClipRectLayer>(layer_bounds,
                                               Clip::antiAliasWithSaveLayer);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_matrix);
  layer->Preroll(preroll_context());

  // Untouched
  EXPECT_EQ(preroll_context()->state_stack.device_cull_rect(), kGiantRect);
  EXPECT_TRUE(preroll_context()->state_stack.is_empty());

  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), mock_layer->paint_bounds());
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_cull_rect(), layer_bounds);
  EXPECT_EQ(mock_layer->parent_matrix(), initial_matrix);
  EXPECT_EQ(mock_layer->parent_mutators(),
            std::vector({Mutator(layer_bounds)}));

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::ClipRectData{
                   clay::ConvertSkityRectToSkRect(layer_bounds),
                   SkClipOp::kIntersect, MockCanvas::kSoft_ClipEdgeStyle}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            SkPaint(), nullptr, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path, child_paint}},
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));

  mock_canvas().reset_draw_calls();

  layer->Paint(checkerboard_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::ClipRectData{
                   clay::ConvertSkityRectToSkRect(layer_bounds),
                   SkClipOp::kIntersect, MockCanvas::kSoft_ClipEdgeStyle}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            SkPaint(), nullptr, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path, child_paint}},
           // start DrawCheckerboard calls
           MockCanvas::DrawCall{
               2, MockCanvas::DrawRectData{clay::ConvertSkityRectToSkRect(
                                               child_bounds),
                                           checkerboard_paint()}},
           // end DrawCheckerboard calls
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(CheckerBoardLayerTest, ClipPathSaveLayerCheckBoard) {
  const skity::Matrix initial_matrix = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds = skity::Rect::MakeXYWH(1.0, 2.0, 2.0, 2.0);
  const skity::Rect layer_bounds = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPath layer_path =
      SkPath()
          .addRect(clay::ConvertSkityRectToSkRect(layer_bounds))
          .addRect(clay::ConvertSkityRectToSkRect(layer_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  const SkPaint clip_paint;
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer =
      std::make_shared<ClipPathLayer>(layer_path, Clip::antiAliasWithSaveLayer);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_matrix);
  layer->Preroll(preroll_context());

  // Untouched
  EXPECT_EQ(preroll_context()->state_stack.device_cull_rect(), kGiantRect);
  EXPECT_TRUE(preroll_context()->state_stack.is_empty());

  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), mock_layer->paint_bounds());
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_cull_rect(), layer_bounds);
  EXPECT_EQ(mock_layer->parent_matrix(), initial_matrix);
  EXPECT_EQ(mock_layer->parent_mutators(), std::vector({Mutator(layer_path)}));

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
           MockCanvas::DrawCall{
               1, MockCanvas::ClipPathData{layer_path, SkClipOp::kIntersect,
                                           MockCanvas::kSoft_ClipEdgeStyle}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            clip_paint, nullptr, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path, child_paint}},
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));

  mock_canvas().reset_draw_calls();

  layer->Paint(checkerboard_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
           MockCanvas::DrawCall{
               1, MockCanvas::ClipPathData{layer_path, SkClipOp::kIntersect,
                                           MockCanvas::kSoft_ClipEdgeStyle}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            clip_paint, nullptr, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path, child_paint}},
           // start DrawCheckerboard calls
           MockCanvas::DrawCall{
               2, MockCanvas::DrawRectData{clay::ConvertSkityRectToSkRect(
                                               child_bounds),
                                           checkerboard_paint()}},
           // end DrawCheckerboard calls
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(CheckerBoardLayerTest, ClipRRectSaveLayerCheckBoard) {
  const skity::Matrix initial_matrix = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds = skity::Rect::MakeXYWH(1.0, 2.0, 2.0, 2.0);
  const skity::Rect layer_bounds = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const skity::RRect layer_rrect =
      skity::RRect::MakeRectXY(layer_bounds, .1, .1);
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  const SkPaint clip_paint;
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ClipRRectLayer>(layer_rrect,
                                                Clip::antiAliasWithSaveLayer);
  layer->Add(mock_layer);

  preroll_context()->state_stack.set_preroll_delegate(initial_matrix);
  layer->Preroll(preroll_context());

  // Untouched
  EXPECT_EQ(preroll_context()->state_stack.device_cull_rect(), kGiantRect);
  EXPECT_TRUE(preroll_context()->state_stack.is_empty());

  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), mock_layer->paint_bounds());
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_cull_rect(), layer_bounds);
  EXPECT_EQ(mock_layer->parent_matrix(), initial_matrix);
  EXPECT_EQ(mock_layer->parent_mutators(), std::vector({Mutator(layer_rrect)}));

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::ClipRRectData{
                   clay::ConvertSkityRRectToSkia(layer_rrect),
                   SkClipOp::kIntersect, MockCanvas::kSoft_ClipEdgeStyle}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            clip_paint, nullptr, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path, child_paint}},
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));

  mock_canvas().reset_draw_calls();

  layer->Paint(checkerboard_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::ClipRRectData{
                   clay::ConvertSkityRRectToSkia(layer_rrect),
                   SkClipOp::kIntersect, MockCanvas::kSoft_ClipEdgeStyle}},
           MockCanvas::DrawCall{
               1, MockCanvas::SaveLayerData{clay::ConvertSkityRectToSkRect(
                                                child_bounds),
                                            clip_paint, nullptr, 2}},
           MockCanvas::DrawCall{
               2, MockCanvas::DrawPathData{child_path, child_paint}},
           // start DrawCheckerboard calls
           MockCanvas::DrawCall{
               2, MockCanvas::DrawRectData{clay::ConvertSkityRectToSkRect(
                                               child_bounds),
                                           checkerboard_paint()}},
           // end DrawCheckerboard calls
           MockCanvas::DrawCall{2, MockCanvas::RestoreData{1}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

#endif
}  // namespace testing
}  // namespace clay
