// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_RENDER_SERVICE_RUNTIME_H_
#define CLAY_COMMON_RENDER_SERVICE_RUNTIME_H_
#include <memory>
#include <utility>

#include "clay/common/raster_thread_group.h"
#include "clay/gfx/shared_image/texture_budget.h"
namespace clay::display {
class RenderServiceRuntime {
 public:
  static std::shared_ptr<RenderServiceRuntime> Acquire(
      uint64_t device, fml::RefPtr<fml::TaskRunner> platform,
      fml::RefPtr<fml::TaskRunner> worker);
  RasterThreadGroup& group() { return group_; }
  const std::shared_ptr<TextureBudget>& budget() const { return budget_; }

 private:
  RenderServiceRuntime(fml::RefPtr<fml::TaskRunner> platform,
                       fml::RefPtr<fml::TaskRunner> worker,
                       std::shared_ptr<TextureBudget> budget)
      : group_(std::move(platform), std::move(worker)),
        budget_(std::move(budget)) {}
  RasterThreadGroup group_;
  std::shared_ptr<TextureBudget> budget_;
};
}  // namespace clay::display
#endif  // CLAY_COMMON_RENDER_SERVICE_RUNTIME_H_
