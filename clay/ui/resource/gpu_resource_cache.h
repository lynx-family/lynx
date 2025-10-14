// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_GPU_RESOURCE_CACHE_H_
#define CLAY_UI_RESOURCE_GPU_RESOURCE_CACHE_H_

#include <set>

#include "clay/gfx/image/skimage_holder.h"

namespace clay {

class GpuResourceCache {
 public:
  GpuResourceCache();
  ~GpuResourceCache() = default;

  void StoreImage(fml::RefPtr<SkImageHolder> img);
  void RemoveImage(fml::RefPtr<SkImageHolder> img);

  void ReduceCacheUsage(bool force = false);
  void ClearCache();

  size_t GetMaxMemoryLimitBytes() const { return max_memory_limit_; }
  void SetMaxMemoryLimitBytes(size_t limit) { max_memory_limit_ = limit; }

  void SetIsLowMemory(bool value);

  void UpdateResourceCacheMaxMemoryLimit(int limit, int low_end_limit);
  bool NeedToRecycle() const;
#ifndef NDEBUG
  void PrintCacheUsage() const;
#endif

 private:
  struct ImageCompare {
    bool operator()(const fml::RefPtr<SkImageHolder>& lhs,
                    const fml::RefPtr<SkImageHolder>& rhs) const {
      if (lhs->timestamp() == rhs->timestamp()) {
        return lhs < rhs;
      }
      return lhs->timestamp() < rhs->timestamp();
    }
  };
  void RecycleInternal();

  std::set<fml::RefPtr<SkImageHolder>, ImageCompare> images_;
  size_t max_memory_limit_;
  size_t current_memory_bytes_ = 0u;
  bool is_low_memory_ = false;
};

}  // namespace clay
#endif  // CLAY_UI_RESOURCE_GPU_RESOURCE_CACHE_H_
