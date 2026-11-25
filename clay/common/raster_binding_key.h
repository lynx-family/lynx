// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_RASTER_BINDING_KEY_H_
#define CLAY_COMMON_RASTER_BINDING_KEY_H_

#include <cstdint>
#include <tuple>

namespace clay::display {
struct RasterBindingKey {
  uint64_t tree_id = 0, tree_generation = 0, group_generation = 0,
           binding_generation = 0;
  uint32_t worker_id = 0;
  bool operator==(const RasterBindingKey& b) const {
    return std::tie(tree_id, tree_generation, group_generation,
                    binding_generation, worker_id) ==
           std::tie(b.tree_id, b.tree_generation, b.group_generation,
                    b.binding_generation, b.worker_id);
  }
  bool operator!=(const RasterBindingKey& b) const { return !(*this == b); }
};
}  // namespace clay::display

#endif  // CLAY_COMMON_RASTER_BINDING_KEY_H_
