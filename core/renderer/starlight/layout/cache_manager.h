// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_CACHE_MANAGER_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_CACHE_MANAGER_H_

#include <memory>

#include "base/include/vector.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/types/layout_types.h"

namespace lynx {
namespace starlight {

class LayoutObject;

struct CacheEntry {
  CompactConstraints resolved_constraints_;
  bool is_valid_ = false;
  float border_bound_width_ = 0.f, border_bound_height_ = 0.f;
};

struct ComposeMeasureCacheEntry {
  ComposeConstraints constraints;
  FloatSize size;
  float inset_width = 0;
  float inset_height = 0;
  bool final_measure = false;
  bool valid = false;
};

struct FindCacheResult {
  const CacheEntry* cache_ = nullptr;
  bool is_cache_in_sync_with_current_state = false;
};

/*
 * The cache slots is 2 inlined and maximum to 8 to cover most cases of complex
 * flex layout. The max size of cache(LAYOUT_RESULT_CACHE_SIZE) is determined by
 * experiment. A hand made deeply nested flex layout test case can reach almost
 * 100% cache reuse rate when the cache size is 8. Which means when cache size 8
 * cache will perform perfectly for almost all actual use case.
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

  bool CanReuseComposeMeasure(const ComposeConstraints& constraints,
                              bool final_measure,
                              const LayoutObject& target) const;
  // Records the key before measuring and clears incompatible CSS results.
  void BeginComposeMeasure(const ComposeConstraints& constraints,
                           bool final_measure, const LayoutObject& target);
  void InsertComposeCacheEntry(const LayoutObject& target);
  void ResetComposeCache();

 private:
  void ResetCssCache();

  base::InlineVector<CacheEntry, 2> cache_data_;
  size_t current_cache_index_ = LAYOUT_RESULT_CACHE_SIZE - 1;
  // Only the latest measurement can reuse the live algorithm/JS placement
  // state. Allocate lazily so CSS cache entries keep their compact layout.
  std::unique_ptr<ComposeMeasureCacheEntry> compose_measure_cache_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_CACHE_MANAGER_H_
