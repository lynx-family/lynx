// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/raster_cache.h"

#include <inttypes.h>

#include <cstddef>
#include <utility>
#include <vector>

#include "clay/common/constants.h"
#include "clay/common/graphics/graphic_meminfo.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer.h"
#include "clay/flow/paint_utils.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/fml/logging.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

namespace {

struct GpuTarget {
  clay::GrCanvas* canvas = nullptr;
#ifndef ENABLE_SKITY
  sk_sp<SkSurface> surface;
#else
  std::unique_ptr<skity::GPURenderTarget> render_target;
#endif  // ENABLE_SKITY
};

GpuTarget CreateGpuTarget(const RasterCache::Context& context, int width,
                          int height) {
#ifndef ENABLE_SKITY
  const SkImageInfo image_info = SkImageInfo::MakeN32Premul(
      width, height, sk_ref_sp(context.dst_color_space));

  auto surface = context.gr_context ? SkSurface::MakeRenderTarget(
                                          context.gr_context,
                                          skgpu::Budgeted::kYes, image_info)
                                    : SkSurface::MakeRaster(image_info);
  if (!surface) {
    return {nullptr, nullptr};
  }
  return {surface->getCanvas(), surface};
#else
  skity::GPURenderTargetDescriptor desc;
  desc.width = width;
  desc.height = height;
  desc.sample_count = 4;

  auto render_target = context.gr_context->CreateRenderTarget(desc);
  if (!render_target) {
    return {nullptr, nullptr};
  }
  return {render_target->GetCanvas(), std::move(render_target)};
#endif  // ENABLE_SKITY
}

}  // namespace

RasterCacheResult::RasterCacheResult(clay::GrImagePtr image,
                                     const skity::Rect& logical_rect,
                                     const char* unused,
                                     const skity::Matrix& matrix)
    : image_(std::move(image)), logical_rect_(logical_rect), matrix_(matrix) {
  flow_id_ = TRACE_FLOW_ID();
  TRACE_EVENT_INSTANT("clay", "RasterCacheResult.Begin",
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_flow_ids(flow_id_);
                      });
}

RasterCacheResult::~RasterCacheResult() {
  TRACE_EVENT_INSTANT("clay", "RasterCacheResult.End",
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_terminating_flow_ids(flow_id_);
                      });
}

void RasterCacheResult::draw(clay::GrCanvas& canvas,
                             const clay::GrPaint* paint) const {
  auto canvas_ptr = &canvas;
  CANVAS_AUTO_RESTORE(canvas_ptr, true);
  auto matrix = CANVAS_GET_TOTAL_MATRIX(canvas_ptr);
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  matrix = RasterCacheUtil::GetIntegralTransCTM(matrix);
#endif
#ifdef ENABLE_RASTER_CACHE_SCALE
  if (RasterCacheUtil::IsMatrixSimilarity(matrix_)) {
    CANVAS_TRANSLATE(canvas_ptr, logical_rect_.Left(), logical_rect_.Top());
    // Canvas already contains the transformations including scale and rotation,
    // so if the cached matrix contains scale, we need to scale back. Blit the
    // image to the target logical rect using DrawImageRect to scale back.
    TRACE_EVENT("clay", "RasterCacheResult::draw()",
                [&](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_flow_ids(flow_id_);
                });
    CANVAS_DRAW_IMAGE_RECT(canvas_ptr, image_,
                           skity::Rect::MakeXYWH(0, 0, logical_rect_.Width(),
                                                 logical_rect_.Height()),
                           SAMPLING_OPTIONS(1, 0), paint);
    return;
  }
#endif  // ENABLE_RASTER_CACHE_SCALE
  auto bounds =
      RasterCacheUtil::GetRoundedOutDeviceBounds(logical_rect_, matrix);
  CANVAS_RESET_MATRIX(canvas_ptr);
  TRACE_EVENT("clay", "RasterCacheResult::draw()",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(flow_id_);
              });
  CANVAS_DRAW_IMAGE(canvas_ptr, image_, bounds, SAMPLING_OPTIONS(0, 0), paint);
}

RasterCache::RasterCache(size_t access_threshold,
                         size_t display_list_cache_limit_per_frame)
    : access_threshold_(access_threshold),
      display_list_cache_limit_per_frame_(display_list_cache_limit_per_frame),
      checkerboard_images_(false) {}

/// @note Procedure doesn't copy all closures.
std::unique_ptr<RasterCacheResult> RasterCache::Rasterize(
    const RasterCache::Context& context,
    const std::function<void(clay::GrCanvas*)>& draw_function,
    const std::function<void(clay::GrCanvas*, const skity::Rect& rect)>&
        draw_checkerboard) const {
  auto matrix = context.matrix;
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  matrix = RasterCacheUtil::GetIntegralTransCTM(matrix);
#endif
#ifdef ENABLE_RASTER_CACHE_SCALE
  if (RasterCacheUtil::IsMatrixSimilarity(matrix)) {
    // With similarity matrix, we only consider the scale factor to cache the
    // texture, and drop translation and rotation transformations (if any). And
    // when the cached texture is drawn, we need to scale back to the original
    // size.
    float scale = RasterCacheUtil::GetScaleFactor(matrix);
    // Create a new surface with the scaled size.
    auto target = CreateGpuTarget(
        context, std::ceil(context.logical_rect.Width() * scale),
        std::ceil(context.logical_rect.Height() * scale));
    clay::GrCanvas* canvas = target.canvas;
    if (!canvas) {
      return nullptr;
    }
    CANVAS_CLEAR(canvas, clay::Color::kTransparent());

    // Scale the canvas.
    CANVAS_SCALE(canvas, scale, scale);
    CANVAS_TRANSLATE(canvas, -context.logical_rect.Left(),
                     -context.logical_rect.Top());

    draw_function(canvas);

    if (checkerboard_images_) {
      draw_checkerboard(canvas, context.logical_rect);
    }

    return std::make_unique<RasterCacheResult>(
#ifndef ENABLE_SKITY
        target.surface->makeImageSnapshot(),
#else
        context.gr_context->MakeSnapshot(std::move(target.render_target)),
#endif  // ENABLE_SKITY
        context.logical_rect, context.flow_type, matrix);
  }
#endif  // ENABLE_RASTER_CACHE_SCALE
  skity::Rect dest_rect =
      RasterCacheUtil::GetRoundedOutDeviceBounds(context.logical_rect, matrix);
  auto target = CreateGpuTarget(context, dest_rect.Width(), dest_rect.Height());
  clay::GrCanvas* canvas = target.canvas;
  if (!canvas) {
    return nullptr;
  }

  CANVAS_CLEAR(canvas, clay::Color::kTransparent());
  CANVAS_TRANSLATE(canvas, -dest_rect.Left(), -dest_rect.Top());
  CANVAS_CONCAT(canvas, matrix);
  draw_function(canvas);

  if (checkerboard_images_) {
    draw_checkerboard(canvas, context.logical_rect);
  }

  return std::make_unique<RasterCacheResult>(
#ifndef ENABLE_SKITY
      target.surface->makeImageSnapshot(),
#else
      context.gr_context->MakeSnapshot(std::move(target.render_target)),
#endif  // ENABLE_SKITY
      context.logical_rect, context.flow_type, matrix);
}

bool RasterCache::UpdateCacheEntry(
    const RasterCacheKeyID& id, const Context& raster_cache_context,
    const std::function<void(clay::GrCanvas*)>& render_function) const {
  RasterCacheKey key = RasterCacheKey(id, raster_cache_context.matrix);
  Entry& entry = cache_[key];
  if (!entry.image) {
    void (*func)(clay::GrCanvas*, const skity::Rect& rect) = DrawCheckerboard;
    entry.image = Rasterize(raster_cache_context, render_function, func);
    if (entry.image != nullptr) {
      switch (id.type()) {
        case RasterCacheKeyType::kPicture: {
          display_list_cached_this_frame_++;
          break;
        }
        default:
          break;
      }
      return true;
    }
  }
  return entry.image != nullptr;
}

RasterCache::CacheInfo RasterCache::MarkSeen(const RasterCacheKeyID& id,
                                             const skity::Matrix& matrix,
                                             bool visible) const {
  RasterCacheKey key = RasterCacheKey(id, matrix);
  Entry& entry = cache_[key];
  entry.encountered_this_frame = true;
  entry.visible_this_frame = visible;
  if (visible || entry.accesses_since_visible > 0) {
    entry.accesses_since_visible++;
  }
  return {entry.accesses_since_visible, entry.image != nullptr};
}

int RasterCache::GetAccessCount(const RasterCacheKeyID& id,
                                const skity::Matrix& matrix) const {
  RasterCacheKey key = RasterCacheKey(id, matrix);
  auto entry = cache_.find(key);
  if (entry != cache_.cend()) {
    return entry->second.accesses_since_visible;
  }
  return -1;
}

bool RasterCache::HasEntry(const RasterCacheKeyID& id,
                           const skity::Matrix& matrix) const {
  RasterCacheKey key = RasterCacheKey(id, matrix);
  if (cache_.find(key) != cache_.cend()) {
    return true;
  }
  return false;
}

bool RasterCache::Draw(const RasterCacheKeyID& id, clay::GrCanvas& canvas,
                       const clay::GrPaint* paint) const {
  auto canvas_ptr = &canvas;
  auto it =
      cache_.find(RasterCacheKey(id, CANVAS_GET_TOTAL_MATRIX(canvas_ptr)));
  if (it == cache_.end()) {
    return false;
  }

  Entry& entry = it->second;

  if (entry.image) {
    entry.image->draw(canvas, paint);
#ifndef NDEBUG
    if (enable_debug_borders_) {
      DrawDebugBorders(canvas_ptr, entry.image->logical_rect());
    }

    if (enable_raster_cache_tag_) {
      DrawRasterCacheTag(canvas_ptr, entry.image->logical_rect().Width() / 2,
                         entry.image->logical_rect().Height() / 2,
                         entry.accesses_since_visible);
    }
#endif  // NDEBUG
    return true;
  }

  return false;
}

void RasterCache::BeginFrame() {
  display_list_cached_this_frame_ = 0;
  picture_metrics_ = {};
  layer_metrics_ = {};
}

void RasterCache::UpdateMetrics() {
  for (auto& it : cache_) {
    Entry& entry = it.second;
    FML_DCHECK(entry.encountered_this_frame);
    if (entry.image) {
      RasterCacheMetrics& metrics = GetMetricsForKind(it.first.kind());
      metrics.in_use_count++;
      metrics.in_use_bytes += entry.image->image_bytes();
    }
    entry.encountered_this_frame = false;
  }
}

void RasterCache::EvictUnusedCacheEntries() {
  std::vector<RasterCacheKey::Map<Entry>::iterator> dead;

  for (auto it = cache_.begin(); it != cache_.end(); ++it) {
    Entry& entry = it->second;
    if (!entry.encountered_this_frame) {
      dead.push_back(it);
    }
  }

  for (auto it : dead) {
    if (it->second.image) {
      RasterCacheMetrics& metrics = GetMetricsForKind(it->first.kind());
      metrics.eviction_count++;
      metrics.eviction_bytes += it->second.image->image_bytes();
    }
    cache_.erase(it);
  }
}

void RasterCache::EndFrame() {
  UpdateMetrics();
  TraceStatsToTimeline();
}

void RasterCache::ClearRasterCacheInfo(std::vector<intptr_t>* cache_address) {
  for (auto address : *cache_address) {
    auto iter = raster_cache_infos_.begin();
    while (iter != raster_cache_infos_.end()) {
      if (iter->cache_address == static_cast<int64_t>(address)) {
        iter = raster_cache_infos_.erase(iter);
      } else {
        iter++;
      }
    }
  }
  cache_address->clear();
}

void RasterCache::Clear() {
  cache_.clear();
  picture_metrics_ = {};
  layer_metrics_ = {};
}

size_t RasterCache::GetCachedEntriesCount() const { return cache_.size(); }

size_t RasterCache::GetLayerCachedEntriesCount() const {
  size_t layer_cached_entries_count = 0;
  for (const auto& item : cache_) {
    if (item.first.kind() == RasterCacheKeyKind::kLayerMetrics) {
      layer_cached_entries_count++;
    }
  }
  return layer_cached_entries_count;
}

size_t RasterCache::GetPictureCachedEntriesCount() const {
  size_t picture_cached_entries_count = 0;
  for (const auto& item : cache_) {
    if (item.first.kind() == RasterCacheKeyKind::kPictureMetrics) {
      picture_cached_entries_count++;
    }
  }
  return picture_cached_entries_count;
}

void RasterCache::SetCheckboardCacheImages(bool checkerboard) {
  if (checkerboard_images_ == checkerboard) {
    return;
  }

  checkerboard_images_ = checkerboard;

  // Clear all existing entries so previously rasterized items (with or without
  // a checkerboard) will be refreshed in subsequent passes.
  Clear();
}

void RasterCache::TraceStatsToTimeline() const {
#if ENABLE_TRACE_PERFETTO
  auto trace_func = [&](const char* name, size_t count) {
    char buf[128] = {0};
    std::sprintf(buf, "RC.%s_%" PRIxPTR, name,
                 reinterpret_cast<uintptr_t>(this));
    TRACE_COUNTER("clay", lynx::perfetto::CounterTrack(buf), count);
  };
  trace_func("LayerCount", layer_metrics_.total_count());
  trace_func("LayerMBytes",
             layer_metrics_.total_bytes() / kMegaByteSizeInBytes);
  trace_func("PictureCount", picture_metrics_.total_count());
  trace_func("PictureMBytes",
             picture_metrics_.total_bytes() / kMegaByteSizeInBytes);
#endif
}

size_t RasterCache::EstimateLayerCacheByteSize() const {
  size_t layer_cache_bytes = 0;
  for (const auto& item : cache_) {
    if (item.first.kind() == RasterCacheKeyKind::kLayerMetrics &&
        item.second.image) {
      layer_cache_bytes += item.second.image->image_bytes();
    }
  }
  return layer_cache_bytes;
}

size_t RasterCache::EstimatePictureCacheByteSize() const {
  size_t picture_cache_bytes = 0;
  for (const auto& item : cache_) {
    if (item.first.kind() == RasterCacheKeyKind::kPictureMetrics &&
        item.second.image) {
      picture_cache_bytes += item.second.image->image_bytes();
    }
  }
  return picture_cache_bytes;
}

RasterCacheMetrics& RasterCache::GetMetricsForKind(RasterCacheKeyKind kind) {
  switch (kind) {
    case RasterCacheKeyKind::kPictureMetrics:
      return picture_metrics_;
    case RasterCacheKeyKind::kLayerMetrics:
      return layer_metrics_;
  }
}

bool RasterCache::RasterCacheInfoChanged() {
  for (auto info : raster_cache_infos_) {
    if (std::find(pre_raster_cache_images_.begin(),
                  pre_raster_cache_images_.end(),
                  info.image.get()) == pre_raster_cache_images_.end()) {
      return true;
    }
  }
  return false;
}

std::vector<RasterCacheInfo>* RasterCache::GetRasterCacheInfo() {
  if (RasterCacheInfoChanged()) {
    pre_raster_cache_images_.clear();
    for (auto info : raster_cache_infos_) {
      pre_raster_cache_images_.emplace_back(info.image.get());
    }
    return &raster_cache_infos_;
  }
  return nullptr;
}

}  // namespace clay
