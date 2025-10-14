// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/image_filter_layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/picture_layer.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/raster_cache_item.h"
#include "clay/flow/testing/gpu_object_layer_test.h"
#include "clay/flow/testing/mock_raster_cache.h"
#include "clay/flow/testing/picture_test_utils.h"
#include "clay/gfx/skia/picture_skia.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/assertions_skia.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"

namespace clay {
namespace testing {

using clay::GPUObject;

TEST(RasterCache, SimpleInitialization) {
  clay::RasterCache cache;
  ASSERT_TRUE(true);
}

TEST(RasterCache, MetricsOmitUnpopulatedEntries) {
  size_t threshold = 2;
  clay::RasterCache cache(threshold);

  skity::Matrix matrix = skity::Matrix();

  auto display_list = GetSamplePicture();
  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();
  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, true, false);

  // 1st access.
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));

  cache.EndFrame();
  ASSERT_EQ(cache.picture_metrics().total_count(), 0u);
  ASSERT_EQ(cache.picture_metrics().total_bytes(), 0u);
  cache.BeginFrame();

  // 2nd access.
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));

  cache.EndFrame();
  ASSERT_EQ(cache.picture_metrics().total_count(), 0u);
  ASSERT_EQ(cache.picture_metrics().total_bytes(), 0u);
  cache.BeginFrame();

  // Now Prepare should cache it.
  ASSERT_TRUE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_TRUE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));

  cache.EndFrame();
  ASSERT_EQ(cache.picture_metrics().total_count(), 1u);
  // 150w * 100h * 4bpp
  ASSERT_EQ(cache.picture_metrics().total_bytes(), 60000u);
}

TEST(RasterCache, ThresholdIsRespectedForDisplayList) {
  size_t threshold = 2;
  clay::RasterCache cache(threshold);

  skity::Matrix matrix = skity::Matrix();

  auto display_list = GetSamplePicture();

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();

  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, true, false);

  // 1st access.
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));

  cache.EndFrame();
  cache.BeginFrame();

  // 2nd access.
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));

  cache.EndFrame();
  cache.BeginFrame();

  // Now Prepare should cache it.
  ASSERT_TRUE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_TRUE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
}

TEST(RasterCache, SetCheckboardCacheImages) {
  size_t threshold = 1;
  clay::RasterCache cache(threshold);

  skity::Matrix matrix = skity::Matrix();
  auto display_list = GetSamplePicture();

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  auto& paint_context = paint_context_holder.paint_context;
  auto dummy_draw_function = [](SkCanvas* canvas) {};
  bool did_draw_checkerboard = false;
  auto draw_checkerboard = [&](SkCanvas* canvas, const skity::Rect&) {
    did_draw_checkerboard = true;
  };
  RasterCache::Context r_context = {
      // clang-format off
      .gr_context         = paint_context.gr_context,
      .dst_color_space    = paint_context.dst_color_space,
      .matrix             = matrix,
      .logical_rect       = clay::ConvertSkRectToSkityRect(display_list->cullRect()),
      .flow_type          = "RasterCacheFlow::DisplayList",
      // clang-format on
  };

  cache.SetCheckboardCacheImages(false);
  cache.Rasterize(r_context, dummy_draw_function, draw_checkerboard);
  ASSERT_FALSE(did_draw_checkerboard);

  cache.SetCheckboardCacheImages(true);
  cache.Rasterize(r_context, dummy_draw_function, draw_checkerboard);
  ASSERT_TRUE(did_draw_checkerboard);
}

TEST(RasterCache, AccessThresholdOfZeroDisablesCachingForSkPicture) {
  size_t threshold = 0;
  clay::RasterCache cache(threshold);

  skity::Matrix matrix = skity::Matrix();

  auto display_list = GetSamplePicture();

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();
  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, true, false);
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
}

TEST(RasterCache, AccessThresholdOfZeroDisablesCachingForDisplayList) {
  size_t threshold = 0;
  clay::RasterCache cache(threshold);

  skity::Matrix matrix = skity::Matrix();

  auto display_list = GetSamplePicture();

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();

  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, true, false);
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
}

TEST(RasterCache, PictureCacheLimitPerFrameIsRespectedWhenZeroForSkPicture) {
  size_t picture_cache_limit_per_frame = 0;
  clay::RasterCache cache(3, picture_cache_limit_per_frame);

  skity::Matrix matrix = skity::Matrix();

  auto display_list = GetSamplePicture();

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();

  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, true, false);
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
}

TEST(RasterCache, PictureCacheLimitPerFrameIsRespectedWhenZeroForDisplayList) {
  size_t picture_cache_limit_per_frame = 0;
  clay::RasterCache cache(3, picture_cache_limit_per_frame);

  skity::Matrix matrix = skity::Matrix();

  auto display_list = GetSamplePicture();

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();

  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, true, false);
  // 1st access.
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
  // 2nd access.
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
  // the picture_cache_limit_per_frame = 0, so don't cache it
  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
}

TEST(RasterCache, EvictUnusedCacheEntries) {
  size_t threshold = 1;
  clay::RasterCache cache(threshold);

  skity::Matrix matrix = skity::Matrix();

  auto display_list_1 = GetSamplePicture();
  auto display_list_2 = GetSamplePicture();

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  PictureRasterCacheItem display_list_item_1(display_list_1.get(),
                                             skity::Vec2{0, 0}, true, false);
  PictureRasterCacheItem display_list_item_2(display_list_2.get(),
                                             skity::Vec2{0, 0}, true, false);

  cache.BeginFrame();
  RasterCacheItemPreroll(display_list_item_1, preroll_context, matrix);
  RasterCacheItemPreroll(display_list_item_2, preroll_context, matrix);
  cache.EvictUnusedCacheEntries();
  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 0u);
  ASSERT_FALSE(
      RasterCacheItemTryToRasterCache(display_list_item_1, paint_context));
  ASSERT_FALSE(
      RasterCacheItemTryToRasterCache(display_list_item_2, paint_context));
  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 0u);
  ASSERT_FALSE(display_list_item_1.Draw(paint_context, &dummy_canvas, &paint));
  ASSERT_FALSE(display_list_item_2.Draw(paint_context, &dummy_canvas, &paint));
  cache.EndFrame();

  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 0u);
  ASSERT_EQ(cache.picture_metrics().total_count(), 0u);
  ASSERT_EQ(cache.picture_metrics().total_bytes(), 0u);

  cache.BeginFrame();
  RasterCacheItemPreroll(display_list_item_1, preroll_context, matrix);
  RasterCacheItemPreroll(display_list_item_2, preroll_context, matrix);
  cache.EvictUnusedCacheEntries();
  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 0u);
  ASSERT_TRUE(
      RasterCacheItemTryToRasterCache(display_list_item_1, paint_context));
  ASSERT_TRUE(
      RasterCacheItemTryToRasterCache(display_list_item_2, paint_context));
  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 120000u);
  ASSERT_TRUE(display_list_item_1.Draw(paint_context, &dummy_canvas, &paint));
  ASSERT_TRUE(display_list_item_2.Draw(paint_context, &dummy_canvas, &paint));
  cache.EndFrame();

  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 120000u);
  ASSERT_EQ(cache.picture_metrics().total_count(), 2u);
  ASSERT_EQ(cache.picture_metrics().total_bytes(), 120000u);

  cache.BeginFrame();
  RasterCacheItemPreroll(display_list_item_1, preroll_context, matrix);
  cache.EvictUnusedCacheEntries();
  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 60000u);
  ASSERT_TRUE(
      RasterCacheItemTryToRasterCache(display_list_item_1, paint_context));
  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 60000u);
  ASSERT_TRUE(display_list_item_1.Draw(paint_context, &dummy_canvas, &paint));
  cache.EndFrame();
  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 60000u);
  ASSERT_EQ(cache.picture_metrics().total_count(), 1u);
  ASSERT_EQ(cache.picture_metrics().total_bytes(), 60000u);

  cache.BeginFrame();
  cache.EvictUnusedCacheEntries();
  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 0u);
  cache.EndFrame();

  ASSERT_EQ(cache.EstimatePictureCacheByteSize(), 0u);
  ASSERT_EQ(cache.picture_metrics().total_count(), 0u);
  ASSERT_EQ(cache.picture_metrics().total_bytes(), 0u);

  cache.BeginFrame();
  ASSERT_FALSE(
      cache.Draw(display_list_item_1.GetId().value(), dummy_canvas, &paint));
  ASSERT_FALSE(display_list_item_1.Draw(paint_context, &dummy_canvas, &paint));
  ASSERT_FALSE(
      cache.Draw(display_list_item_2.GetId().value(), dummy_canvas, &paint));
  ASSERT_FALSE(display_list_item_2.Draw(paint_context, &dummy_canvas, &paint));
  cache.EndFrame();
}

TEST(RasterCache, ComputeDeviceRectBasedOnFractionalTranslation) {
  skity::Rect logical_rect = skity::Rect::MakeLTRB(0, 0, 300.2, 300.3);
  skity::Matrix ctm = skity::Matrix(2.0, 0, 0, 0, 2.0, 0, 0, 0, 1);
  auto result = RasterCacheUtil::GetDeviceBounds(logical_rect, ctm);
  ASSERT_EQ(result, skity::Rect::MakeLTRB(0.0, 0.0, 600.4, 600.6));
}

// Construct a cache result whose device target rectangle rounds out to be one
// pixel wider than the cached image.  Verify that it can be drawn without
// triggering any assertions.
TEST(RasterCache, DeviceRectRoundOutForDisplayList) {
  size_t threshold = 1;
  clay::RasterCache cache(threshold);

  SkRect logical_rect = SkRect::MakeLTRB(28, 0, 354.56731, 310.288);
  SkPictureRecorder recorder;
  SkCanvas* canvas_build = recorder.beginRecording(logical_rect);
  SkPaint paint_build;
  paint_build.setColor(SK_ColorRED);
  canvas_build->drawRect(logical_rect, paint_build);
  sk_sp<SkPicture> display_list = recorder.finishRecordingAsPicture();

  skity::Matrix ctm = skity::Matrix(1.3312, 0, 233, 0, 1.3312, 206, 0, 0, 1);
  SkPaint paint;

  SkCanvas canvas(1000, 1000, nullptr);
  canvas.setMatrix(clay::ConvertSkityMatrixToSkMatrix(ctm));

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, ctm);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();
  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, true, false);

  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, ctm));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &canvas, &paint));

  cache.EndFrame();
  cache.BeginFrame();

  ASSERT_TRUE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, ctm));
  ASSERT_TRUE(display_list_item.Draw(paint_context, &canvas, &paint));

  canvas.translate(248, 0);
  ASSERT_TRUE(cache.Draw(display_list_item.GetId().value(), canvas, &paint));
  ASSERT_TRUE(display_list_item.Draw(paint_context, &canvas, &paint));
}

TEST(RasterCache, NestedOpCountMetricUsedForDisplayList) {
  size_t threshold = 1;
  clay::RasterCache cache(threshold);

  skity::Matrix matrix = skity::Matrix();

  auto display_list = GetSampleNestedPicture();
  ASSERT_EQ(display_list->approximateOpCount(), 1);
  ASSERT_EQ(display_list->approximateOpCount(true), 36);

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();

  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, false, false);

  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));

  cache.EndFrame();
  cache.BeginFrame();

  ASSERT_TRUE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_TRUE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));
}

TEST(RasterCache, NaiveComplexityScoringDisplayList) {
  PictureComplexityCalculator* calculator =
      PictureNaiveComplexityCalculator::GetInstance();

  size_t threshold = 1;
  clay::RasterCache cache(threshold);

  skity::Matrix matrix = skity::Matrix();

  // Five raster ops will not be cached
  auto display_list = GetSamplePicture(5);
  ASSERT_EQ(display_list->approximateOpCount(), 5);
  unsigned int complexity_score = calculator->Compute(display_list.get());

  ASSERT_EQ(complexity_score, 5u);
  ASSERT_FALSE(calculator->ShouldBeCached(complexity_score));

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  cache.BeginFrame();

  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, false, false);

  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));

  cache.EndFrame();
  cache.BeginFrame();

  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item.Draw(paint_context, &dummy_canvas, &paint));

  // Six raster ops should be cached
  display_list = GetSamplePicture(6);
  ASSERT_EQ(display_list->approximateOpCount(), 6);
  complexity_score = calculator->Compute(display_list.get());

  ASSERT_EQ(complexity_score, 6u);
  ASSERT_TRUE(calculator->ShouldBeCached(complexity_score));

  PictureRasterCacheItem display_list_item_2 = PictureRasterCacheItem(
      display_list.get(), skity::Vec2{0, 0}, false, false);
  cache.BeginFrame();

  ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item_2, preroll_context, paint_context, matrix));
  ASSERT_FALSE(display_list_item_2.Draw(paint_context, &dummy_canvas, &paint));

  cache.EndFrame();
  cache.BeginFrame();

  ASSERT_TRUE(RasterCacheItemPrerollAndTryToRasterCache(
      display_list_item_2, preroll_context, paint_context, matrix));
  ASSERT_TRUE(display_list_item_2.Draw(paint_context, &dummy_canvas, &paint));
}

TEST(RasterCache, DisplayListWithSingularMatrixIsNotCached) {
  size_t threshold = 2;
  clay::RasterCache cache(threshold);

  skity::Matrix matrices[] = {
      skity::Matrix::Scale(0, 1),
      skity::Matrix::Scale(1, 0),
      skity::Matrix::Skew(1, 1),
  };
  int matrix_count = sizeof(matrices) / sizeof(matrices[0]);

  auto display_list = GetSamplePicture();

  SkCanvas dummy_canvas(1000, 1000);
  SkPaint paint;

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, skity::Matrix());
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  PrerollContextHolder preroll_context_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  PaintContextHolder paint_context_holder = GetSamplePaintContextHolder(
      paint_state_stack, &cache, &raster_time, &ui_time);
  auto& preroll_context = preroll_context_holder.preroll_context;
  auto& paint_context = paint_context_holder.paint_context;

  PictureRasterCacheItem display_list_item(display_list.get(),
                                           skity::Vec2{0, 0}, true, false);

  for (int i = 0; i < 10; i++) {
    cache.BeginFrame();

    for (int j = 0; j < matrix_count; j++) {
      display_list_item.set_matrix(matrices[j]);
      ASSERT_FALSE(RasterCacheItemPrerollAndTryToRasterCache(
          display_list_item, preroll_context, paint_context, matrices[j]));
    }

    for (int j = 0; j < matrix_count; j++) {
      dummy_canvas.setMatrix(clay::ConvertSkityMatrixToSkMatrix(matrices[j]));
      ASSERT_FALSE(
          display_list_item.Draw(paint_context, &dummy_canvas, &paint));
    }

    cache.EndFrame();
  }
}

TEST(RasterCache, PrepareLayerTransform) {
  SkRect child_bounds = SkRect::MakeLTRB(10, 10, 50, 50);
  SkPath child_path = SkPath().addOval(child_bounds);
  auto child_layer = MockLayer::Make(child_path);
  auto blur_filter =
      std::make_shared<DlBlurImageFilter>(5, 5, DlTileMode::kClamp);
  auto blur_layer = std::make_shared<ImageFilterLayer>(blur_filter);
  skity::Matrix matrix = skity::Matrix::Scale(2, 2);
  auto transform_layer = std::make_shared<TransformLayer>(matrix);
  skity::Matrix cache_matrix = skity::Matrix::Translate(-20, -20);
  cache_matrix.PreConcat(matrix);
  child_layer->set_expected_paint_matrix(cache_matrix);

  blur_layer->Add(child_layer);
  transform_layer->Add(blur_layer);

  size_t threshold = 2;
  MockRasterCache cache(threshold);
  SkCanvas dummy_canvas(1000, 1000);

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, matrix);
  LayerStateStack paint_state_stack;
  preroll_state_stack.set_delegate(&dummy_canvas);

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  std::vector<RasterCacheItem*> cache_items;

  cache.BeginFrame();

  auto preroll_holder = GetSamplePrerollContextHolder(
      preroll_state_stack, &cache, &raster_time, &ui_time);
  preroll_holder.preroll_context.raster_cached_entries = &cache_items;
  transform_layer->Preroll(&preroll_holder.preroll_context);

  auto paint_holder = GetSamplePaintContextHolder(paint_state_stack, &cache,
                                                  &raster_time, &ui_time);

  cache.EvictUnusedCacheEntries();
  LayerTree::TryToRasterCache(
      *preroll_holder.preroll_context.raster_cached_entries,
      &paint_holder.paint_context);

  // Condition tested inside MockLayer::Paint against expected paint matrix.
}

TEST(RasterCache, RasterCacheKeyHashFunction) {
  RasterCacheKey::Map<int> map;
  auto hash_function = map.hash_function();
  skity::Matrix matrix = skity::Matrix();
  uint64_t id = 5;
  RasterCacheKey layer_key(id, RasterCacheKeyType::kLayer, matrix);
  RasterCacheKey display_list_key(id, RasterCacheKeyType::kPicture, matrix);
  RasterCacheKey layer_children_key(id, RasterCacheKeyType::kLayerChildren,
                                    matrix);

  auto layer_cache_key_id = RasterCacheKeyID(id, RasterCacheKeyType::kLayer);
  auto layer_hash_code = hash_function(layer_key);
  ASSERT_EQ(layer_hash_code, layer_cache_key_id.GetHash());

  auto display_list_cache_key_id =
      RasterCacheKeyID(id, RasterCacheKeyType::kPicture);
  auto display_list_hash_code = hash_function(display_list_key);
  ASSERT_EQ(display_list_hash_code, display_list_cache_key_id.GetHash());

  auto layer_children_cache_key_id =
      RasterCacheKeyID(id, RasterCacheKeyType::kLayerChildren);
  auto layer_children_hash_code = hash_function(layer_children_key);
  ASSERT_EQ(layer_children_hash_code, layer_children_cache_key_id.GetHash());
}

TEST(RasterCache, RasterCacheKeySameID) {
  RasterCacheKey::Map<int> map;
  skity::Matrix matrix = skity::Matrix();
  uint64_t id = 5;
  RasterCacheKey layer_key(id, RasterCacheKeyType::kLayer, matrix);
  RasterCacheKey display_list_key(id, RasterCacheKeyType::kPicture, matrix);
  RasterCacheKey layer_children_key(id, RasterCacheKeyType::kLayerChildren,
                                    matrix);
  map[layer_key] = 100;
  map[display_list_key] = 300;
  map[layer_children_key] = 400;

  ASSERT_EQ(map[layer_key], 100);
  ASSERT_EQ(map[display_list_key], 300);
  ASSERT_EQ(map[layer_children_key], 400);
}

TEST(RasterCache, RasterCacheKeySameType) {
  RasterCacheKey::Map<int> map;
  skity::Matrix matrix = skity::Matrix();

  RasterCacheKeyType type = RasterCacheKeyType::kLayer;
  RasterCacheKey layer_first_key(5, type, matrix);
  RasterCacheKey layer_second_key(10, type, matrix);
  RasterCacheKey layer_third_key(15, type, matrix);
  map[layer_first_key] = 50;
  map[layer_second_key] = 100;
  map[layer_third_key] = 150;
  ASSERT_EQ(map[layer_first_key], 50);
  ASSERT_EQ(map[layer_second_key], 100);
  ASSERT_EQ(map[layer_third_key], 150);

  type = RasterCacheKeyType::kPicture;
  RasterCacheKey picture_first_key(20, type, matrix);
  RasterCacheKey picture_second_key(25, type, matrix);
  RasterCacheKey picture_third_key(30, type, matrix);
  map[picture_first_key] = 200;
  map[picture_second_key] = 250;
  map[picture_third_key] = 300;
  ASSERT_EQ(map[picture_first_key], 200);
  ASSERT_EQ(map[picture_second_key], 250);
  ASSERT_EQ(map[picture_third_key], 300);

  type = RasterCacheKeyType::kPicture;
  RasterCacheKey display_list_first_key(35, type, matrix);
  RasterCacheKey display_list_second_key(40, type, matrix);
  RasterCacheKey display_list_third_key(45, type, matrix);
  map[display_list_first_key] = 350;
  map[display_list_second_key] = 400;
  map[display_list_third_key] = 450;
  ASSERT_EQ(map[display_list_first_key], 350);
  ASSERT_EQ(map[display_list_second_key], 400);
  ASSERT_EQ(map[display_list_third_key], 450);

  type = RasterCacheKeyType::kLayerChildren;
  RasterCacheKeyID foo = RasterCacheKeyID(10, RasterCacheKeyType::kLayer);
  RasterCacheKeyID bar = RasterCacheKeyID(20, RasterCacheKeyType::kLayer);
  RasterCacheKeyID baz = RasterCacheKeyID(30, RasterCacheKeyType::kLayer);
  RasterCacheKey layer_children_first_key(
      RasterCacheKeyID({foo, bar, baz}, type), matrix);
  RasterCacheKey layer_children_second_key(
      RasterCacheKeyID({foo, baz, bar}, type), matrix);
  RasterCacheKey layer_children_third_key(
      RasterCacheKeyID({baz, bar, foo}, type), matrix);
  map[layer_children_first_key] = 100;
  map[layer_children_second_key] = 200;
  map[layer_children_third_key] = 300;
  ASSERT_EQ(map[layer_children_first_key], 100);
  ASSERT_EQ(map[layer_children_second_key], 200);
  ASSERT_EQ(map[layer_children_third_key], 300);
}

TEST(RasterCache, RasterCacheKeyIDEqual) {
  RasterCacheKeyID first = RasterCacheKeyID(1, RasterCacheKeyType::kLayer);
  RasterCacheKeyID second = RasterCacheKeyID(2, RasterCacheKeyType::kLayer);
  RasterCacheKeyID third =
      RasterCacheKeyID(1, RasterCacheKeyType::kLayerChildren);

  ASSERT_NE(first, second);
  ASSERT_NE(first, third);
  ASSERT_NE(second, third);

  RasterCacheKeyID fourth =
      RasterCacheKeyID({first, second}, RasterCacheKeyType::kLayer);
  RasterCacheKeyID fifth =
      RasterCacheKeyID({first, second}, RasterCacheKeyType::kLayerChildren);
  RasterCacheKeyID sixth =
      RasterCacheKeyID({second, first}, RasterCacheKeyType::kLayerChildren);
  ASSERT_NE(fourth, fifth);
  ASSERT_NE(fifth, sixth);
}

TEST(RasterCache, RasterCacheKeyIDHashCode) {
  uint64_t foo = 1;
  uint64_t bar = 2;
  RasterCacheKeyID first = RasterCacheKeyID(foo, RasterCacheKeyType::kLayer);
  RasterCacheKeyID second = RasterCacheKeyID(bar, RasterCacheKeyType::kLayer);
  std::size_t first_hash = first.GetHash();
  std::size_t second_hash = second.GetHash();

  ASSERT_EQ(first_hash, fml::HashCombine(foo, RasterCacheKeyType::kLayer));
  ASSERT_EQ(second_hash, fml::HashCombine(bar, RasterCacheKeyType::kLayer));

  RasterCacheKeyID third =
      RasterCacheKeyID({first, second}, RasterCacheKeyType::kLayerChildren);
  RasterCacheKeyID fourth =
      RasterCacheKeyID({second, first}, RasterCacheKeyType::kLayerChildren);
  std::size_t third_hash = third.GetHash();
  std::size_t fourth_hash = fourth.GetHash();

  ASSERT_EQ(third_hash, fml::HashCombine(RasterCacheKeyID::kDefaultUniqueID,
                                         RasterCacheKeyType::kLayerChildren,
                                         first.GetHash(), second.GetHash()));
  ASSERT_EQ(fourth_hash, fml::HashCombine(RasterCacheKeyID::kDefaultUniqueID,
                                          RasterCacheKeyType::kLayerChildren,
                                          second.GetHash(), first.GetHash()));

  // Verify that the cached hash code is correct.
  ASSERT_EQ(first_hash, first.GetHash());
  ASSERT_EQ(second_hash, second.GetHash());
  ASSERT_EQ(third_hash, third.GetHash());
  ASSERT_EQ(fourth_hash, fourth.GetHash());
}

using RasterCacheTest = GPUObjectLayerTest;

TEST_F(RasterCacheTest, RasterCacheKeyIDLayerChildrenIds) {
  auto layer = std::make_shared<ContainerLayer>();

  const SkPath child_path = SkPath().addRect(SkRect::MakeWH(5.0f, 5.0f));
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  layer->Add(mock_layer);

  auto display_list = GetSamplePicture();
  clay::DynamicOps ops;
  auto picture_skia =
      fml::MakeRefCounted<clay::PictureSkia>(display_list, std::move(ops));
  auto display_list_layer = std::make_shared<PictureLayer>(
      skity::Vec2(0.0f, 0.0f), GPUObject(picture_skia, unref_queue()), false,
      false);
  auto id = display_list->uniqueID();
  layer->Add(display_list_layer);

  auto ids = RasterCacheKeyID::LayerChildrenIds(layer.get()).value();
  std::vector<RasterCacheKeyID> expected_ids;
  expected_ids.emplace_back(
      RasterCacheKeyID(mock_layer->unique_id(), RasterCacheKeyType::kLayer));
  expected_ids.emplace_back(RasterCacheKeyID(id, RasterCacheKeyType::kPicture));
  ASSERT_EQ(expected_ids[0], mock_layer->caching_key_id());
  ASSERT_EQ(expected_ids[1], display_list_layer->caching_key_id());
  ASSERT_EQ(ids, expected_ids);
}

}  // namespace testing
}  // namespace clay
