// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_CACHEABLE_LAYER_H_
#define CLAY_FLOW_LAYERS_CACHEABLE_LAYER_H_

#include <memory>

#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer_raster_cache_item.h"

namespace clay {

class AutoCache {
 public:
  AutoCache(RasterCacheItem* raster_cache_item, PrerollContext* context,
            const skity::Matrix& matrix);

  void ShouldNotBeCached() { raster_cache_item_ = nullptr; }

  ~AutoCache();

 private:
  inline bool IsCacheEnabled();
  RasterCacheItem* raster_cache_item_ = nullptr;
  PrerollContext* context_ = nullptr;
  const skity::Matrix matrix_;
};

class CacheableContainerLayer : public ContainerLayer {
 public:
  explicit CacheableContainerLayer(
      int layer_cached_threshold =
          RasterCacheUtil::kMinimumRendersBeforeCachingFilterLayer,
      bool can_cache_children = false);

  const LayerRasterCacheItem* raster_cache_item() const {
    return layer_raster_cache_item_.get();
  }

 protected:
  void UpdateRasterCacheItem(int layer_cached_threshold,
                             bool can_cache_children);

  std::unique_ptr<LayerRasterCacheItem> layer_raster_cache_item_;
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_CACHEABLE_LAYER_H_
