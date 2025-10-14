// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_TESTING_MOCK_RASTER_CACHE_H_
#define CLAY_FLOW_TESTING_MOCK_RASTER_CACHE_H_

#include <memory>
#include <vector>

#include "clay/flow/layers/layer.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/raster_cache_item.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkPicture.h"

namespace clay {
namespace testing {

/**
 * @brief A RasterCacheResult implementation that represents a cached Layer or
 * SkPicture without the overhead of storage.
 *
 * This implementation is used by MockRasterCache only for testing proper usage
 * of the RasterCache in layer unit tests.
 */
class MockRasterCacheResult : public RasterCacheResult {
 public:
  explicit MockRasterCacheResult(skity::Rect device_rect);

  void draw(SkCanvas& canvas, const SkPaint* paint = nullptr) const override{};

  skity::Vec2 image_dimensions() const override {
    return skity::Vec2(device_rect_.Width(), device_rect_.Height());
  };

  int64_t image_bytes() const override {
    return image_dimensions().x * image_dimensions().y *
           SkColorTypeBytesPerPixel(kBGRA_8888_SkColorType);
  }

 private:
  skity::Rect device_rect_;
};

static std::vector<RasterCacheItem*> raster_cache_items_;

/**
 * @brief A RasterCache implementation that simulates the act of rendering a
 * Layer or SkPicture without the overhead of rasterization or pixel storage.
 * This implementation is used only for testing proper usage of the RasterCache
 * in layer unit tests.
 */
class MockRasterCache : public RasterCache {
 public:
  explicit MockRasterCache(
      size_t access_threshold = 3,
      size_t picture_and_display_list_cache_limit_per_frame =
          RasterCacheUtil::kDefaultPictureAndDispLayListCacheLimitPerFrame)
      : RasterCache(access_threshold,
                    picture_and_display_list_cache_limit_per_frame) {
    preroll_state_stack_.set_preroll_delegate(skity::Matrix());
    paint_state_stack_.set_delegate(&mock_canvas_);
  }

  void AddMockLayer(int width, int height);
  void AddMockPicture(int width, int height);

 private:
  LayerStateStack preroll_state_stack_;
  LayerStateStack paint_state_stack_;
  MockCanvas mock_canvas_;
  SkColorSpace* color_space_ = mock_canvas_.imageInfo().colorSpace();
  MutatorsStack mutators_stack_;
  FixedRefreshRateStopwatch raster_time_;
  FixedRefreshRateStopwatch ui_time_;
  std::shared_ptr<DrawableImageRegistry> drawable_image_registry_;
  PrerollContext preroll_context_ = {
      // clang-format off
      .raster_cache                  = this,
      .gr_context                    = nullptr,
      .compositor_state              = nullptr,
      .state_stack                   = preroll_state_stack_,
      .dst_color_space               = color_space_,
      .surface_needs_readback        = false,
      .raster_time                   = raster_time_,
      .ui_time                       = ui_time_,
      .drawable_image_registry       = drawable_image_registry_,
      .frame_device_pixel_ratio      = 1.0f,
      .has_platform_view             = false,
      .has_drawable_image_layer      = false,
      .raster_cached_entries         = &raster_cache_items_
      // clang-format on
  };

  PaintContext paint_context_ = {
      // clang-format off
      .state_stack                   = paint_state_stack_,
      .canvas                        = nullptr,
      .gr_context                    = nullptr,
      .dst_color_space               = color_space_,
      .compositor_state              = nullptr,
      .raster_time                   = raster_time_,
      .ui_time                       = ui_time_,
      .drawable_image_registry       = drawable_image_registry_,
      .raster_cache                  = nullptr,
      .frame_device_pixel_ratio      = 1.0f,
      // clang-format on
  };
};

struct PrerollContextHolder {
  PrerollContext preroll_context;
  sk_sp<SkColorSpace> srgb;
};

struct PaintContextHolder {
  PaintContext paint_context;
  sk_sp<SkColorSpace> srgb;
};

PrerollContextHolder GetSamplePrerollContextHolder(
    LayerStateStack& state_stack, RasterCache* raster_cache,
    FixedRefreshRateStopwatch* raster_time, FixedRefreshRateStopwatch* ui_time);

PaintContextHolder GetSamplePaintContextHolder(
    LayerStateStack& state_stack, RasterCache* raster_cache,
    FixedRefreshRateStopwatch* raster_time, FixedRefreshRateStopwatch* ui_time);

bool RasterCacheItemPrerollAndTryToRasterCache(
    PictureRasterCacheItem& display_list_item, PrerollContext& context,
    PaintContext& paint_context, const skity::Matrix& matrix);

void RasterCacheItemPreroll(PictureRasterCacheItem& display_list_item,
                            PrerollContext& context,
                            const skity::Matrix& matrix);

bool RasterCacheItemTryToRasterCache(PictureRasterCacheItem& display_list_item,
                                     PaintContext& paint_context);

}  // namespace testing
}  // namespace clay

#endif  // CLAY_FLOW_TESTING_MOCK_RASTER_CACHE_H_
