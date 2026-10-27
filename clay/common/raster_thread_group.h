// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_RASTER_THREAD_GROUP_H_
#define CLAY_COMMON_RASTER_THREAD_GROUP_H_
#include <memory>

#include "clay/common/raster_binding.h"
namespace clay::display {
class RasterThreadGroup {
 public:
  RasterThreadGroup(fml::RefPtr<fml::TaskRunner> platform,
                    fml::RefPtr<fml::TaskRunner> worker);
  // announce_layout runs on the worker immediately before its tree update.
  std::shared_ptr<RasterBinding> RegisterRasterizer(
      uint64_t tree_id, uint64_t tree_generation,
      std::function<bool()> initialize, std::function<void(bool)> ready,
      std::function<void(uint64_t)> announce_layout,
      std::function<void(RasterBindingKey)> prepare);
  void UnregisterRasterizer(const std::shared_ptr<RasterBinding>& binding,
                            std::function<void(std::function<void()>)> release,
                            std::function<void()> drained);

 private:
  fml::RefPtr<fml::TaskRunner> platform_, worker_;
  uint64_t generation_, next_binding_ = 0;
};
}  // namespace clay::display
#endif  // CLAY_COMMON_RASTER_THREAD_GROUP_H_
