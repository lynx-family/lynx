// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_INSTRUMENTATION_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_INSTRUMENTATION_SERVICE_H_

#include <memory>
#include <vector>

#include "clay/common/service/service.h"

namespace clay {
class FrameTimingCollector;
}  // namespace clay

namespace clay {

struct RasterCacheInfo;
class FrameTiming;
class FrameTimingListener;

class InstrumentationService
    : public clay::Service<InstrumentationService, clay::Owner::kPlatform> {
 public:
  static std::shared_ptr<InstrumentationService> Create();

  void UpdateRasterCacheInfo(
      const std::vector<RasterCacheInfo>& raster_cache_info);
  void OnFrameRasterized(const FrameTiming& timing);

  const std::shared_ptr<clay::FrameTimingCollector>& GetFrameTimingCollector()
      const;

  void AddFrameTimingListener(std::shared_ptr<FrameTimingListener> listener);
  void RemoveFrameTimingListener(std::shared_ptr<FrameTimingListener> listener);

 private:
  void OnInit(clay::ServiceManager& service_manager,
              const clay::PlatformServiceContext& ctx) override;
  void OnDestroy() override;

  Shell* shell_ = nullptr;
  std::vector<std::shared_ptr<FrameTimingListener>> frame_timing_listeners_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_INSTRUMENTATION_SERVICE_H_
