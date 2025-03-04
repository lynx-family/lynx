// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_CACHE_MANAGER_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_CACHE_MANAGER_H_

#include <array>

#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/types/layout_types.h"

namespace lynx {
namespace starlight {

class LayoutObject;

struct CacheEntry {
  Constraints resolved_constraints_;
  float border_bound_width_ = 0.f, border_bound_height_ = 0.f;
  bool is_valid_ = false;
};

struct FindCacheResult {
  const CacheEntry* cache_ = nullptr;
  bool is_cache_in_sync_with_current_state = false;
};

/*
 * The cache slots is 8 to cover most cases of complex flex layout.
 * The size of cache is determined by experiment.
 * A hand made deeply nested flex layout test case can reach almost 100% cache
 * reuse rate when the cache size is 8. Which
 * means when cache size 8 cache will perform perfectly for almost all actual
 * use case.
 */
constexpr size_t LAYOUT_RESULT_CACHE_SIZE = 8;

class CacheManager {
 public:
  void ResetCache();
  void InsertCacheEntry(const Constraints& constraints,
                        float border_bound_width, float border_bound_height);
  /*
   * Find the cache entry which can be used for size
   *
   * @return first: the cache entry that meet the requirement, second: if the
   * cached entry is the last layout result
   */
  FindCacheResult FindAvailableCacheEntry(const Constraints& constraints,
                                          LayoutObject& target) const;

 private:
  std::array<CacheEntry, LAYOUT_RESULT_CACHE_SIZE> cache_data_;
  size_t current_cache_index_ = LAYOUT_RESULT_CACHE_SIZE - 1;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_CACHE_MANAGER_H_
