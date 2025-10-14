// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_RASTER_CACHE_ITEM_H_
#define CLAY_FLOW_RASTER_CACHE_ITEM_H_

#include <memory>
#include <optional>

#include "clay/flow/raster_cache_key.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

struct PrerollContext;
struct PaintContext;
class RasterCache;
class LayerRasterCacheItem;
class PictureRasterCacheItem;

class RasterCacheItem {
 public:
  enum CacheState {
    kNone = 0,
    kCurrent,
    kChildren,
  };

  explicit RasterCacheItem(RasterCacheKeyID key_id,
                           CacheState cache_state = CacheState::kNone,
                           unsigned child_entries = 0)
      : key_id_(key_id),
        cache_state_(cache_state),
        child_items_(child_entries) {}

  virtual void PrerollSetup(PrerollContext* context,
                            const skity::Matrix& matrix) = 0;

  virtual void PrerollFinalize(PrerollContext* context,
                               const skity::Matrix& matrix) = 0;

  virtual bool Draw(const PaintContext& context,
                    const clay::GrPaint* paint) const = 0;

  virtual bool Draw(const PaintContext& context, clay::GrCanvas* canvas,
                    const clay::GrPaint* paint) const = 0;

  virtual std::optional<RasterCacheKeyID> GetId() const { return key_id_; }

  virtual bool TryToPrepareRasterCache(const PaintContext& context,
                                       bool parent_cached = false) const = 0;

  unsigned child_items() const { return child_items_; }

  void set_matrix(const skity::Matrix& matrix) { matrix_ = matrix; }

  CacheState cache_state() const { return cache_state_; }

  bool need_caching() const { return cache_state_ != CacheState::kNone; }

  bool has_been_cached() const { return has_been_cached_; }

  virtual ~RasterCacheItem() = default;

 protected:
  // The id for cache the layer self.
  RasterCacheKeyID key_id_;
  CacheState cache_state_ = CacheState::kNone;
  mutable skity::Matrix matrix_;
  unsigned child_items_;

  mutable bool has_been_cached_ = false;
};

}  // namespace clay

#endif  // CLAY_FLOW_RASTER_CACHE_ITEM_H_
