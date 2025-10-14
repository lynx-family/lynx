// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_RASTER_CACHE_H_
#define CLAY_FLOW_RASTER_CACHE_H_

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/graphic_meminfo.h"
#include "clay/flow/layers/picture_complexity.h"
#include "clay/flow/raster_cache_key.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

enum CacheStrategy {
  None,
  ForceCache,
  // not cache and not use cache.
  NotCache,
};

enum class RasterCacheLayerStrategy { kLayer, kLayerChildren };

class RasterCacheResult {
 public:
  RasterCacheResult(clay::GrImagePtr image, const skity::Rect& logical_rect,
                    const char* type, const skity::Matrix& matrix);

  virtual ~RasterCacheResult();

  virtual void draw(clay::GrCanvas& canvas, const clay::GrPaint* paint) const;

  virtual skity::Vec2 image_dimensions() const {
    return image_ ? skity::Vec2(IMAGE_WIDTH(image_), IMAGE_HEIGHT(image_))
                  : skity::Vec2(0, 0);
  };

  virtual int64_t image_bytes() const {
    return image_ ? IMAGE_BYTE_SIZE(image_) : 0;
  };

  const skity::Rect& logical_rect() const { return logical_rect_; }

 private:
  clay::GrImagePtr image_;
  skity::Rect logical_rect_;
  uint64_t flow_id_;
  skity::Matrix matrix_;
};

class Layer;
class RasterCacheItem;
struct PrerollContext;
struct PaintContext;

struct RasterCacheMetrics {
  /**
   * The number of cache entries with images evicted in this frame.
   */
  size_t eviction_count = 0;

  /**
   * The size of all of the images evicted in this frame.
   */
  size_t eviction_bytes = 0;

  /**
   * The number of cache entries with images used in this frame.
   */
  size_t in_use_count = 0;

  /**
   * The size of all of the images used in this frame.
   */
  size_t in_use_bytes = 0;

  /**
   * The total cache entries that had images during this frame.
   */
  size_t total_count() const { return in_use_count; }

  /**
   * The size of all of the cached images during this frame.
   */
  size_t total_bytes() const { return in_use_bytes; }
};

/**
 * RasterCache is used to cache rasterized layers or display lists to improve
 * performance.
 *
 * Life cycle of RasterCache methods:
 * - Preroll stage
 *   - LayerTree::Preroll - for each Layer in the tree:
 *     - RasterCacheItem::PrerollSetup
 *         At the start of each layer's preroll, add cache items to
 *         `PrerollContext::raster_cached_entries`.
 *     - RasterCacheItem::PrerollFinalize
 *         At the end of each layer's preroll, may mark cache entries as
 *         encountered by the current frame.
 * - Paint stage
 *   - RasterCache::EvictUnusedCacheEntries
 *       Evict cached images that are no longer used.
 *   - LayerTree::TryToPrepareRasterCache
 *       Create cache image for each cache entry if it does not exist.
 *   - LayerTree::Paint - for each layer in the tree:
 *       If layers or display lists are cached as cached images, the method
 *       `RasterCache::Draw` will be used to draw those cache images.
 *   - RasterCache::EndFrame:
 *       Computes used counts and memory then reports cache metrics.
 */
class RasterCache {
 public:
  struct Context {
    clay::GrContext* gr_context;
#ifndef ENABLE_SKITY
    const SkColorSpace* dst_color_space;
#endif  // ENABLE_SKITY
    const skity::Matrix& matrix;
    const skity::Rect& logical_rect;
    const char* flow_type;
  };

  struct CacheInfo {
    const size_t accesses_since_visible;
    const bool has_image;
  };

  std::unique_ptr<RasterCacheResult> Rasterize(
      const RasterCache::Context& context,
      const std::function<void(clay::GrCanvas*)>& draw_function,
      const std::function<void(clay::GrCanvas*, const skity::Rect& rect)>&
          draw_checkerboard) const;

  explicit RasterCache(
      size_t access_threshold = 2,
      size_t picture_and_display_list_cache_limit_per_frame =
          RasterCacheUtil::kDefaultPictureAndDispLayListCacheLimitPerFrame);

  virtual ~RasterCache() = default;

  // Draws this item if it should be rendered from the cache and returns
  // true iff it was successfully drawn. Typically this should only fail
  // if the item was disabled due to conditions discovered during |Preroll|
  // or if the attempt to populate the entry failed due to bounds overflow
  // conditions.
  bool Draw(const RasterCacheKeyID& id, clay::GrCanvas& canvas,
            const clay::GrPaint* paint) const;

  bool HasEntry(const RasterCacheKeyID& id, const skity::Matrix&) const;

  void BeginFrame();

  void EvictUnusedCacheEntries();

  void EndFrame();

  void Clear();

  void SetCheckboardCacheImages(bool checkerboard);

  const RasterCacheMetrics& picture_metrics() const { return picture_metrics_; }
  const RasterCacheMetrics& layer_metrics() const { return layer_metrics_; }

  size_t GetCachedEntriesCount() const;

  /**
   * Return the number of map entries in the layer cache regardless of whether
   * the entries have been populated with an image.
   */
  size_t GetLayerCachedEntriesCount() const;

  /**
   * Return the number of map entries in the picture caches (SkPicture and
   * DisplayList) regardless of whether the entries have been populated with
   * an image.
   */
  size_t GetPictureCachedEntriesCount() const;

  /**
   * @brief Estimate how much memory is used by picture raster cache entries in
   * bytes, including cache entries in the SkPicture cache and the DisplayList
   * cache.
   *
   * Only SkImage's memory usage is counted as other objects are often much
   * smaller compared to SkImage. SkImageInfo::computeMinByteSize is used to
   * estimate the SkImage memory usage.
   */
  size_t EstimatePictureCacheByteSize() const;

  /**
   * @brief Estimate how much memory is used by layer raster cache entries in
   * bytes.
   *
   * Only SkImage's memory usage is counted as other objects are often much
   * smaller compared to SkImage. SkImageInfo::computeMinByteSize is used to
   * estimate the SkImage memory usage.
   */
  size_t EstimateLayerCacheByteSize() const;

  /**
   * @brief Return the number of frames that a picture must be prepared
   * before it will be cached. If the number is 0, then no picture will
   * ever be cached.
   *
   * If the number is one, then it must be prepared and drawn on 1 frame
   * and it will then be cached on the next frame if it is prepared.
   */
  size_t access_threshold() const { return access_threshold_; }

  bool GenerateNewCacheInThisFrame() const {
    // Disabling caching when access_threshold is zero is historic behavior.
    return access_threshold_ != 0 && display_list_cached_this_frame_ <
                                         display_list_cache_limit_per_frame_;
  }

  /**
   * @brief The entry whose RasterCacheKey is generated by RasterCacheKeyID
   * and matrix is marked as encountered by the current frame. The entry
   * will be created if it does not exist. Optionally the entry will be marked
   * as visible in the current frame if the caller determines that it
   * intersects the cull rect. The access_count of the entry will be
   * increased if it is visible, or if it was ever visible.
   * @return the number of times the entry has been hit since it was created.
   * For a new entry that will be 1 if it is visible, or zero if non-visible.
   */
  CacheInfo MarkSeen(const RasterCacheKeyID& id, const skity::Matrix& matrix,
                     bool visible) const;

  /**
   * Returns the access count (i.e. accesses_since_visible) for the given
   * entry in the cache, or -1 if no such entry exists.
   */
  int GetAccessCount(const RasterCacheKeyID& id,
                     const skity::Matrix& matrix) const;

  bool UpdateCacheEntry(
      const RasterCacheKeyID& id, const Context& raster_cache_context,
      const std::function<void(clay::GrCanvas*)>& render_function) const;

  void set_needs_build_all_caches(bool needs_build_all_caches) {
    needs_build_all_caches_ = needs_build_all_caches;
  }

  bool RasterCacheInfoChanged();
  std::vector<RasterCacheInfo>* GetRasterCacheInfo();

  void ClearRasterCacheInfo(std::vector<intptr_t>* cache_address);

 private:
  struct Entry {
    bool encountered_this_frame = false;
    bool visible_this_frame = false;
    size_t accesses_since_visible = 0;
    std::unique_ptr<RasterCacheResult> image;
  };

  void UpdateMetrics();

  RasterCacheMetrics& GetMetricsForKind(RasterCacheKeyKind kind);

  std::vector<intptr_t> sweep_cache_address_;

  const size_t access_threshold_;
  const size_t display_list_cache_limit_per_frame_;
  mutable size_t display_list_cached_this_frame_ = 0;
  RasterCacheMetrics layer_metrics_;
  RasterCacheMetrics picture_metrics_;
  mutable RasterCacheKey::Map<Entry> cache_;
  bool checkerboard_images_;
  bool needs_build_all_caches_ = false;
  std::vector<RasterCacheInfo> raster_cache_infos_;
  std::vector<clay::GrImage*> pre_raster_cache_images_;

  void TraceStatsToTimeline() const;

#if !defined(NDEBUG)
  // Will add brown solid borders to every entry.
  bool enable_debug_borders_ = false;
  // Will add accesses count as a tag to every raster cache area.
  bool enable_raster_cache_tag_ = false;
#endif

  friend class RasterCacheItem;
  friend class LayerRasterCacheItem;

  BASE_DISALLOW_COPY_AND_ASSIGN(RasterCache);
};

}  // namespace clay

#endif  // CLAY_FLOW_RASTER_CACHE_H_
