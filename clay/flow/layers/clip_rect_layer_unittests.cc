// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/compositor/compositor_state.h"
#include "clay/flow/layers/clip_rect_layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/layers/platform_view_layer.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/mock_canvas.h"

namespace clay {
namespace testing {

using ClipRectLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(ClipRectLayerTest, ClipNoneBehaviorDies) {
  EXPECT_DEATH_IF_SUPPORTED(
      auto clip = std::make_shared<ClipRectLayer>(kEmptyRect, Clip::none),
      "clip_behavior != Clip::none");
}

TEST_F(ClipRectLayerTest, PaintingEmptyLayerDies) {
  auto layer = std::make_shared<ClipRectLayer>(kEmptyRect, Clip::hardEdge);

  layer->Preroll(preroll_context());

  // Untouched
  EXPECT_EQ(preroll_context()->state_stack.device_cull_rect(), kGiantRect);
  EXPECT_TRUE(preroll_context()->state_stack.is_empty());

  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(ClipRectLayerTest, PaintBeforePrerollDies) {
  const skity::Rect layer_bounds = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  auto layer = std::make_shared<ClipRectLayer>(layer_bounds, Clip::hardEdge);
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->child_paint_bounds(), kEmptyRect);
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(ClipRectLayerTest, PaintingCulledLayerDies) {
  const skity::Matrix initial_matrix = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds = skity::Rect::MakeXYWH(1.0, 2.0, 2.0, 2.0);
  const skity::Rect layer_bounds = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  const skity::Rect distant_bounds =
      skity::Rect::MakeXYWH(100.0, 100.0, 10.0, 10.0);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ClipRectLayer>(layer_bounds, Clip::hardEdge);
  layer->Add(mock_layer);

  // Cull these children
  preroll_context()->state_stack.set_preroll_delegate(distant_bounds,
                                                      initial_matrix);
  layer->Preroll(preroll_context());

  // Untouched
  EXPECT_EQ(preroll_context()->state_stack.device_cull_rect(), distant_bounds);
  EXPECT_TRUE(preroll_context()->state_stack.is_empty());

  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_cull_rect(), kEmptyRect);
  EXPECT_EQ(mock_layer->parent_matrix(), initial_matrix);
  EXPECT_EQ(mock_layer->parent_mutators(),
            std::vector({Mutator(layer_bounds)}));

  auto mutator = paint_context().state_stack.save();
  mutator.clipRect(distant_bounds, false);
  EXPECT_FALSE(mock_layer->needs_painting(paint_context()));
  EXPECT_FALSE(layer->needs_painting(paint_context()));
  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(ClipRectLayerTest, ChildOutsideBounds) {
  const skity::Matrix initial_matrix = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect local_cull_bounds =
      skity::Rect::MakeXYWH(0.0, 0.0, 2.0, 4.0);
  const skity::Rect device_cull_bounds =
      initial_matrix.MapRect(local_cull_bounds);
  const skity::Rect child_bounds = skity::Rect::MakeXYWH(2.5, 5.0, 4.5, 4.0);
  const skity::Rect clip_rect = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ClipRectLayer>(clip_rect, Clip::hardEdge);
  layer->Add(mock_layer);

  skity::Rect clip_cull_rect = local_cull_bounds;
  ASSERT_TRUE(clip_cull_rect.Intersect(clip_rect));
  skity::Rect clip_layer_bounds = child_bounds;
  ASSERT_TRUE(clip_layer_bounds.Intersect(clip_rect));

  // Set up both contexts to cull clipped child
  preroll_context()->state_stack.set_preroll_delegate(device_cull_bounds,
                                                      initial_matrix);
  paint_context().canvas->clipRect(
      clay::ConvertSkityRectToSkRect(device_cull_bounds));
  paint_context().canvas->concat(
      clay::ConvertSkityMatrixToSkMatrix(initial_matrix));

  layer->Preroll(preroll_context());
  // Untouched
  EXPECT_EQ(preroll_context()->state_stack.device_cull_rect(),
            device_cull_bounds);
  EXPECT_EQ(preroll_context()->state_stack.local_cull_rect(),
            local_cull_bounds);
  EXPECT_TRUE(preroll_context()->state_stack.is_empty());

  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), clip_layer_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_EQ(mock_layer->parent_cull_rect(), clip_cull_rect);
  EXPECT_EQ(mock_layer->parent_matrix(), initial_matrix);
  EXPECT_EQ(mock_layer->parent_mutators(), std::vector({Mutator(clip_rect)}));

  EXPECT_FALSE(layer->needs_painting(paint_context()));
  EXPECT_FALSE(mock_layer->needs_painting(paint_context()));
  // Top level layer not visible so calling layer->Paint()
  // would trip an FML_DCHECK
}

TEST_F(ClipRectLayerTest, FullyContainedChild) {
  const skity::Matrix initial_matrix = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect child_bounds = skity::Rect::MakeXYWH(1.0, 2.0, 2.0, 2.0);
  const skity::Rect layer_bounds = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ClipRectLayer>(layer_bounds, Clip::hardEdge);
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
                   SkClipOp::kIntersect, MockCanvas::kHard_ClipEdgeStyle}},
           MockCanvas::DrawCall{
               1, MockCanvas::DrawPathData{child_path, child_paint}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

TEST_F(ClipRectLayerTest, PartiallyContainedChild) {
  const skity::Matrix initial_matrix = skity::Matrix::Translate(0.5f, 1.0f);
  const skity::Rect local_cull_bounds =
      skity::Rect::MakeXYWH(0.0, 0.0, 4.0, 5.5);
  const skity::Rect device_cull_bounds =
      initial_matrix.MapRect(local_cull_bounds);
  const skity::Rect child_bounds = skity::Rect::MakeXYWH(2.5, 5.0, 4.5, 4.0);
  const skity::Rect clip_rect = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kYellow);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ClipRectLayer>(clip_rect, Clip::hardEdge);
  layer->Add(mock_layer);

  skity::Rect clip_cull_rect = clip_rect;
  ASSERT_TRUE(clip_cull_rect.Intersect(local_cull_bounds));
  skity::Rect clip_layer_bounds = clip_rect;
  ASSERT_TRUE(clip_layer_bounds.Intersect(child_bounds));

  // Cull child
  preroll_context()->state_stack.set_preroll_delegate(device_cull_bounds,
                                                      initial_matrix);
  layer->Preroll(preroll_context());

  // Untouched
  EXPECT_EQ(preroll_context()->state_stack.device_cull_rect(),
            device_cull_bounds);
  EXPECT_TRUE(preroll_context()->state_stack.is_empty());

  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), clip_layer_bounds);
  EXPECT_EQ(layer->child_paint_bounds(), child_bounds);
  EXPECT_TRUE(mock_layer->needs_painting(paint_context()));
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_EQ(mock_layer->parent_cull_rect(), clip_cull_rect);
  EXPECT_EQ(mock_layer->parent_matrix(), initial_matrix);
  EXPECT_EQ(mock_layer->parent_mutators(), std::vector({Mutator(clip_rect)}));

  layer->Paint(paint_context());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector(
          {MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
           MockCanvas::DrawCall{
               1,
               MockCanvas::ClipRectData{
                   clay::ConvertSkityRectToSkRect(clip_rect),
                   SkClipOp::kIntersect, MockCanvas::kHard_ClipEdgeStyle}},
           MockCanvas::DrawCall{
               1, MockCanvas::DrawPathData{child_path, child_paint}},
           MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}}));
}

static bool ReadbackResult(PrerollContext* context, Clip clip_behavior,
                           const std::shared_ptr<Layer>& child, bool before) {
  const skity::Rect layer_bounds = skity::Rect::MakeXYWH(0.5, 1.0, 5.0, 6.0);
  auto layer = std::make_shared<ClipRectLayer>(layer_bounds, clip_behavior);
  if (child != nullptr) {
    layer->Add(child);
  }
  context->surface_needs_readback = before;
  layer->Preroll(context);
  return context->surface_needs_readback;
}

TEST_F(ClipRectLayerTest, Readback) {
  PrerollContext* context = preroll_context();
  SkPath path;
  SkPaint paint;

  const Clip hard = Clip::hardEdge;
  const Clip soft = Clip::antiAlias;
  const Clip save_layer = Clip::antiAliasWithSaveLayer;

  std::shared_ptr<MockLayer> nochild;
  auto reader = std::make_shared<MockLayer>(path, paint);
  reader->set_fake_reads_surface(true);
  auto nonreader = std::make_shared<MockLayer>(path, paint);

  // No children, no prior readback -> no readback after
  EXPECT_FALSE(ReadbackResult(context, hard, nochild, false));
  EXPECT_FALSE(ReadbackResult(context, soft, nochild, false));
  EXPECT_FALSE(ReadbackResult(context, save_layer, nochild, false));

  // No children, prior readback -> readback after
  EXPECT_TRUE(ReadbackResult(context, hard, nochild, true));
  EXPECT_TRUE(ReadbackResult(context, soft, nochild, true));
  EXPECT_TRUE(ReadbackResult(context, save_layer, nochild, true));

  // Non readback child, no prior readback -> no readback after
  EXPECT_FALSE(ReadbackResult(context, hard, nonreader, false));
  EXPECT_FALSE(ReadbackResult(context, soft, nonreader, false));
  EXPECT_FALSE(ReadbackResult(context, save_layer, nonreader, false));

  // Non readback child, prior readback -> readback after
  EXPECT_TRUE(ReadbackResult(context, hard, nonreader, true));
  EXPECT_TRUE(ReadbackResult(context, soft, nonreader, true));
  EXPECT_TRUE(ReadbackResult(context, save_layer, nonreader, true));

  // Readback child, no prior readback -> readback after unless SaveLayer
  EXPECT_TRUE(ReadbackResult(context, hard, reader, false));
  EXPECT_TRUE(ReadbackResult(context, soft, reader, false));
  EXPECT_FALSE(ReadbackResult(context, save_layer, reader, false));

  // Readback child, prior readback -> readback after
  EXPECT_TRUE(ReadbackResult(context, hard, reader, true));
  EXPECT_TRUE(ReadbackResult(context, soft, reader, true));
  EXPECT_TRUE(ReadbackResult(context, save_layer, reader, true));
}

TEST_F(ClipRectLayerTest, OpacityInheritance) {
  auto path1 = SkPath().addRect({10, 10, 30, 30});
  auto mock1 = MockLayer::MakeOpacityCompatible(path1);
  skity::Rect clip_rect = skity::Rect::MakeWH(500, 500);
  auto clip_rect_layer =
      std::make_shared<ClipRectLayer>(clip_rect, Clip::hardEdge);
  clip_rect_layer->Add(mock1);

  // ClipRectLayer will pass through compatibility from a compatible child
  PrerollContext* context = preroll_context();
  clip_rect_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);

  auto path2 = SkPath().addRect({40, 40, 50, 50});
  auto mock2 = MockLayer::MakeOpacityCompatible(path2);
  clip_rect_layer->Add(mock2);

  // ClipRectLayer will pass through compatibility from multiple
  // non-overlapping compatible children
  clip_rect_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);

  auto path3 = SkPath().addRect({20, 20, 40, 40});
  auto mock3 = MockLayer::MakeOpacityCompatible(path3);
  clip_rect_layer->Add(mock3);

  // ClipRectLayer will not pass through compatibility from multiple
  // overlapping children even if they are individually compatible
  clip_rect_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags, 0);

  {
    // ClipRectLayer(aa with saveLayer) will always be compatible
    auto clip_path_savelayer = std::make_shared<ClipRectLayer>(
        clip_rect, Clip::antiAliasWithSaveLayer);
    clip_path_savelayer->Add(mock1);
    clip_path_savelayer->Add(mock2);

    // Double check first two children are compatible and non-overlapping
    clip_path_savelayer->Preroll(context);
    EXPECT_EQ(context->renderable_state_flags, Layer::kSaveLayerRenderFlags);

    // Now add the overlapping child and test again, should still be compatible
    clip_path_savelayer->Add(mock3);
    clip_path_savelayer->Preroll(context);
    EXPECT_EQ(context->renderable_state_flags, Layer::kSaveLayerRenderFlags);
  }

  // An incompatible, but non-overlapping child for the following tests
  auto path4 = SkPath().addRect({60, 60, 70, 70});
  auto mock4 = MockLayer::Make(path4);

  {
    // ClipRectLayer with incompatible child will not be compatible
    auto clip_rect_bad_child =
        std::make_shared<ClipRectLayer>(clip_rect, Clip::hardEdge);
    clip_rect_bad_child->Add(mock1);
    clip_rect_bad_child->Add(mock2);

    // Double check first two children are compatible and non-overlapping
    clip_rect_bad_child->Preroll(context);
    EXPECT_EQ(context->renderable_state_flags,
              LayerStateStack::kCallerCanApplyOpacity);

    clip_rect_bad_child->Add(mock4);

    // The third child is non-overlapping, but not compatible so the
    // TransformLayer should end up incompatible
    clip_rect_bad_child->Preroll(context);
    EXPECT_EQ(context->renderable_state_flags, 0);
  }

  {
    // ClipRectLayer(aa with saveLayer) will always be compatible
    auto clip_path_savelayer_bad_child = std::make_shared<ClipRectLayer>(
        clip_rect, Clip::antiAliasWithSaveLayer);
    clip_path_savelayer_bad_child->Add(mock1);
    clip_path_savelayer_bad_child->Add(mock2);

    // Double check first two children are compatible and non-overlapping
    clip_path_savelayer_bad_child->Preroll(context);
    EXPECT_EQ(context->renderable_state_flags, Layer::kSaveLayerRenderFlags);

    // Now add the incompatible child and test again, should still be compatible
    clip_path_savelayer_bad_child->Add(mock4);
    clip_path_savelayer_bad_child->Preroll(context);
    EXPECT_EQ(context->renderable_state_flags, Layer::kSaveLayerRenderFlags);
  }
}

TEST_F(ClipRectLayerTest, OpacityInheritancePainting) {
  auto path1 = SkPath().addRect({10, 10, 30, 30});
  auto mock1 = MockLayer::MakeOpacityCompatible(path1);
  auto path2 = SkPath().addRect({40, 40, 50, 50});
  auto mock2 = MockLayer::MakeOpacityCompatible(path2);
  skity::Rect clip_rect = skity::Rect::MakeWH(500, 500);
  auto clip_rect_layer =
      std::make_shared<ClipRectLayer>(clip_rect, Clip::antiAlias);
  clip_rect_layer->Add(mock1);
  clip_rect_layer->Add(mock2);

  // ClipRectLayer will pass through compatibility from multiple
  // non-overlapping compatible children
  PrerollContext* context = preroll_context();
  clip_rect_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);

  int opacity_alpha = 0x7F;
  skity::Vec2 offset = skity::Vec2(10, 10);
  auto opacity_layer = std::make_shared<OpacityLayer>(opacity_alpha, offset);
  opacity_layer->Add(clip_rect_layer);
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
        canvas->save();
        canvas->clipRect(clay::ConvertSkityRectToSkRect(clip_rect),
                         SkClipOp::kIntersect, true);
        {
          SkPaint paint1;
          paint1.setAlpha(opacity_alpha);
          canvas->drawPath(path1, paint1);
        }
        {
          SkPaint paint2;
          paint2.setAlpha(opacity_alpha);
          canvas->drawPath(path2, paint2);
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

TEST_F(ClipRectLayerTest, OpacityInheritanceSaveLayerPainting) {
  auto path1 = SkPath().addRect({10, 10, 30, 30});
  auto mock1 = MockLayer::MakeOpacityCompatible(path1);
  auto path2 = SkPath().addRect({20, 20, 40, 40});
  auto mock2 = MockLayer::MakeOpacityCompatible(path2);
  auto children_bounds = path1.getBounds();
  children_bounds.join(path2.getBounds());
  SkRect clip_rect = SkRect::MakeWH(500, 500);
  auto clip_rect_layer = std::make_shared<ClipRectLayer>(
      clay::ConvertSkRectToSkityRect(clip_rect), Clip::antiAliasWithSaveLayer);
  clip_rect_layer->Add(mock1);
  clip_rect_layer->Add(mock2);

  // ClipRectLayer will pass through compatibility from multiple
  // non-overlapping compatible children
  PrerollContext* context = preroll_context();
  clip_rect_layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags, Layer::kSaveLayerRenderFlags);

  int opacity_alpha = 0x7F;
  skity::Vec2 offset = skity::Vec2(10, 10);
  auto opacity_layer = std::make_shared<OpacityLayer>(opacity_alpha, offset);
  opacity_layer->Add(clip_rect_layer);
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
        canvas->save();
        canvas->clipRect(clip_rect, SkClipOp::kIntersect, true);
        SkPaint paint;
        paint.setColor(opacity_alpha << 24);
        canvas->saveLayer(&children_bounds, &paint);
        {
          SkPaint paint1;
          paint1.setColor(0xFF000000);
          canvas->drawPath(path1, paint1);
        }
        { canvas->drawPath(path2, SkPaint()); }
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

TEST_F(ClipRectLayerTest, LayerCached) {
  auto path1 = SkPath().addRect({10, 10, 30, 30});
  auto mock1 = MockLayer::MakeOpacityCompatible(path1);
  skity::Rect clip_rect = skity::Rect::MakeWH(500, 500);
  auto layer =
      std::make_shared<ClipRectLayer>(clip_rect, Clip::antiAliasWithSaveLayer);
  layer->Add(mock1);

  auto initial_transform = SkMatrix::Translate(50.0, 25.5);
  SkMatrix cache_ctm = initial_transform;
  SkCanvas cache_canvas;
  cache_canvas.setMatrix(cache_ctm);

  use_mock_raster_cache();
  preroll_context()->state_stack.set_preroll_delegate(
      clay::ConvertSkMatrixToSkityMatrix(initial_transform));

  const auto* clip_cache_item = layer->raster_cache_item();

  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());

  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(clip_cache_item->cache_state(), RasterCacheItem::CacheState::kNone);

  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)0);
  EXPECT_EQ(clip_cache_item->cache_state(), RasterCacheItem::CacheState::kNone);

  layer->Preroll(preroll_context());
  LayerTree::TryToRasterCache(cacheable_items(), &paint_context());
  EXPECT_EQ(raster_cache()->GetLayerCachedEntriesCount(), (size_t)1);
  EXPECT_EQ(clip_cache_item->cache_state(),
            RasterCacheItem::CacheState::kCurrent);
  SkPaint paint;
  EXPECT_TRUE(raster_cache()->Draw(clip_cache_item->GetId().value(),
                                   cache_canvas, &paint));
}

TEST_F(ClipRectLayerTest, EmptyClipDoesNotCullPlatformView) {
  const skity::Vec2 view_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 view_size = skity::Vec2(8.0f, 8.0f);
  const int64_t view_id = 42;
  auto platform_view =
      std::make_shared<PlatformViewLayer>(view_offset, view_size, view_id);

  auto clip = std::make_shared<ClipRectLayer>(kEmptyRect, Clip::hardEdge);
  clip->Add(platform_view);

  CompositorState compositor_state({64, 64});
  preroll_context()->compositor_state = &compositor_state;
  paint_context().compositor_state = &compositor_state;

  clip->Preroll(preroll_context());
  // cspell:words prerolled
  EXPECT_EQ(compositor_state.GetCompositionOrder(),
            std::vector<int64_t>({view_id}));

  clip->Paint(paint_context());
  EXPECT_EQ(paint_context().canvas,
            compositor_state.GetSlices()[view_id]->canvas());
}

}  // namespace testing
}  // namespace clay
