// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/raster_cache_key.h"

#include <optional>

#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer.h"
namespace clay {

//

std::optional<std::vector<RasterCacheKeyID>> RasterCacheKeyID::LayerChildrenIds(
    const Layer* layer) {
  FML_DCHECK(layer->as_container_layer());
  auto& children_layers = layer->as_container_layer()->layers();
  auto children_count = children_layers.size();
  if (children_count == 0) {
    return std::nullopt;
  }
  std::vector<RasterCacheKeyID> ids;
  std::transform(
      children_layers.begin(), children_layers.end(), std::back_inserter(ids),
      [](auto& layer) -> RasterCacheKeyID { return layer->caching_key_id(); });
  return ids;
}

}  // namespace clay
