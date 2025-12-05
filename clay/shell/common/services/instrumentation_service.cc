// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/instrumentation_service.h"

#include "clay/shell/common/frame_timing_listener.h"
#include "clay/shell/common/shell.h"

namespace clay {

std::shared_ptr<InstrumentationService> InstrumentationService::Create() {
  return std::make_shared<InstrumentationService>();
}

void InstrumentationService::OnInit(clay::ServiceManager& service_manager,
                                    const clay::PlatformServiceContext& ctx) {
  shell_ = ctx.shell;
}

void InstrumentationService::OnDestroy() { frame_timing_listeners_.clear(); }

const std::shared_ptr<clay::PerfCollector>&
InstrumentationService::GetPerfCollector() const {
  return shell_->GetPerfCollector();
}

void InstrumentationService::AddFrameTimingListener(
    std::shared_ptr<FrameTimingListener> listener) {
  if (!listener) {
    return;
  }
  frame_timing_listeners_.push_back(listener);
}

void InstrumentationService::UpdateRasterCacheInfo(
    const std::vector<RasterCacheInfo>& raster_cache_info) {
  shell_->UpdateRasterInfo(raster_cache_info);
}

void InstrumentationService::OnFrameRasterized(const FrameTiming& timing) {
  if (!frame_timing_listeners_.empty()) {
    int64_t frame_start_time_in_ns =
        timing.Get(FrameTiming::kVsyncStart).ToEpochDelta().ToNanoseconds();
    int64_t frame_finish_time_in_ns =
        timing.Get(FrameTiming::kRasterFinish).ToEpochDelta().ToNanoseconds();
    for (const auto& listener : frame_timing_listeners_) {
      listener->OnFrameTiming(frame_start_time_in_ns, frame_finish_time_in_ns);
    }
  }

  if (auto callback = shell_->GetSettings().frame_rasterized_callback) {
    callback(timing);
  }
}

}  // namespace clay
