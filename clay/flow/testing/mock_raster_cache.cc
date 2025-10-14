// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/mock_raster_cache.h"

#include "clay/flow/layers/layer.h"
#include "clay/flow/layers/picture_raster_cache_item.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/raster_cache_item.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkPoint.h"

namespace clay {
namespace testing {

MockRasterCacheResult::MockRasterCacheResult(skity::Rect device_rect)
    : RasterCacheResult(nullptr, skity::Rect::MakeEmpty(),
                        "RasterCacheFlow::test", skity::Matrix()),
      device_rect_(device_rect) {}

void MockRasterCache::AddMockLayer(int width, int height) {
  skity::Matrix ctm = skity::Matrix();
  SkPath path;
  path.addRect(100, 100, 100 + width, 100 + height);
  int layer_cached_threshold = 1;
  MockCacheableLayer layer =
      MockCacheableLayer(path, SkPaint(), layer_cached_threshold);
  layer.Preroll(&preroll_context_);
  layer.raster_cache_item()->TryToPrepareRasterCache(paint_context_);
  RasterCache::Context r_context = {
      // clang-format off
      .gr_context         = preroll_context_.gr_context,
      .dst_color_space    = preroll_context_.dst_color_space,
      .matrix             = ctm,
      .logical_rect       = layer.paint_bounds(),
      // clang-format on
  };
  UpdateCacheEntry(
      RasterCacheKeyID(layer.unique_id(), RasterCacheKeyType::kLayer),
      r_context, [&](SkCanvas* canvas) {
        skity::Rect cache_rect = RasterCacheUtil::GetDeviceBounds(
            r_context.logical_rect, r_context.matrix);
        return std::make_unique<MockRasterCacheResult>(cache_rect);
      });
}

void MockRasterCache::AddMockPicture(int width, int height) {
  FML_DCHECK(access_threshold() > 0);
  skity::Matrix ctm = skity::Matrix();
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(
      SkRect::MakeLTRB(0, 0, 200 + width, 200 + height));
  SkPath path;
  path.addRect(100, 100, 100 + width, 100 + height);
  canvas->drawPath(path, SkPaint());
  sk_sp<SkPicture> picture = recorder.finishRecordingAsPicture();

  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  LayerStateStack state_stack;
  PaintContextHolder holder =
      GetSamplePaintContextHolder(state_stack, this, &raster_time, &ui_time);
  holder.paint_context.dst_color_space = color_space_;

  PictureRasterCacheItem display_list_item(picture.get(), skity::Vec2{0, 0},
                                           true, false);
  auto id = picture->uniqueID();
  for (size_t i = 0; i < access_threshold(); i++) {
    AutoCache(&display_list_item, &preroll_context_, ctm);
  }
  RasterCache::Context r_context = {
      // clang-format off
      .gr_context         = preroll_context_.gr_context,
      .dst_color_space    = preroll_context_.dst_color_space,
      .matrix             = ctm,
      .logical_rect       = clay::ConvertSkRectToSkityRect(picture->cullRect()),
      // clang-format on
  };
  UpdateCacheEntry(RasterCacheKeyID(id, RasterCacheKeyType::kPicture),
                   r_context, [&](SkCanvas* canvas) {
                     skity::Rect cache_rect = RasterCacheUtil::GetDeviceBounds(
                         r_context.logical_rect, r_context.matrix);
                     return std::make_unique<MockRasterCacheResult>(cache_rect);
                   });
}

PrerollContextHolder GetSamplePrerollContextHolder(
    LayerStateStack& state_stack, RasterCache* raster_cache,
    FixedRefreshRateStopwatch* raster_time,
    FixedRefreshRateStopwatch* ui_time) {
  sk_sp<SkColorSpace> srgb = SkColorSpace::MakeSRGB();

  PrerollContextHolder holder = {
      {
          // clang-format off
          .raster_cache                  = raster_cache,
          .gr_context                    = nullptr,
          .compositor_state              = nullptr,
          .state_stack                   = state_stack,
          .dst_color_space               = srgb.get(),
          .surface_needs_readback        = false,
          .raster_time                   = *raster_time,
          .ui_time                       = *ui_time,
          .drawable_image_registry       = nullptr,
          .frame_device_pixel_ratio      = 1.0f,
          .has_platform_view             = false,
          .has_drawable_image_layer      = false,
          .raster_cached_entries         = &raster_cache_items_,
          // clang-format on
      },
      srgb};

  return holder;
}

PaintContextHolder GetSamplePaintContextHolder(
    LayerStateStack& state_stack, RasterCache* raster_cache,
    FixedRefreshRateStopwatch* raster_time,
    FixedRefreshRateStopwatch* ui_time) {
  sk_sp<SkColorSpace> srgb = SkColorSpace::MakeSRGB();
  PaintContextHolder holder = {// clang-format off
    {
        .state_stack                   = state_stack,
        .canvas                        = nullptr,
        .gr_context                    = nullptr,
        .dst_color_space               = srgb.get(),
        .compositor_state              = nullptr,
        .raster_time                   = *raster_time,
        .ui_time                       = *ui_time,
        .drawable_image_registry       = nullptr,
        .raster_cache                  = raster_cache,
        .frame_device_pixel_ratio      = 1.0f,
    },
                               // clang-format on
                               srgb};

  return holder;
}

bool RasterCacheItemPrerollAndTryToRasterCache(
    PictureRasterCacheItem& display_list_item, PrerollContext& context,
    PaintContext& paint_context, const skity::Matrix& matrix) {
  RasterCacheItemPreroll(display_list_item, context, matrix);
  context.raster_cache->EvictUnusedCacheEntries();
  return RasterCacheItemTryToRasterCache(display_list_item, paint_context);
}

void RasterCacheItemPreroll(PictureRasterCacheItem& display_list_item,
                            PrerollContext& context,
                            const skity::Matrix& matrix) {
  display_list_item.PrerollSetup(&context, matrix);
  display_list_item.PrerollFinalize(&context, matrix);
}

bool RasterCacheItemTryToRasterCache(PictureRasterCacheItem& display_list_item,
                                     PaintContext& paint_context) {
  if (display_list_item.cache_state() ==
      RasterCacheItem::CacheState::kCurrent) {
    return display_list_item.TryToPrepareRasterCache(paint_context);
  }
  return false;
}

}  // namespace testing
}  // namespace clay
