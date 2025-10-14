// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_INSTRUMENTATION_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_INSTRUMENTATION_SERVICE_H_

#include <memory>
#include <vector>

#include "clay/common/service/service.h"

namespace clay {
class PerfCollector;
}

namespace clay {

struct RasterCacheInfo;
class FrameTiming;

class InstrumentationService
    : public clay::Service<InstrumentationService, clay::Owner::kPlatform> {
 public:
  static std::shared_ptr<InstrumentationService> Create();

  void UpdateRasterCacheInfo(
      const std::vector<RasterCacheInfo>& raster_cache_info);
  void OnFrameRasterized(const FrameTiming& timing);

  const std::shared_ptr<clay::PerfCollector>& GetPerfCollector() const;

 private:
  void OnInit(clay::ServiceManager& service_manager,
              const clay::PlatformServiceContext& ctx) override;
  Shell* shell_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_INSTRUMENTATION_SERVICE_H_
