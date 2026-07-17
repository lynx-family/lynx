// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/instrumentation_service.h"  // nogncheck
#include "clay/shell/common/services/raster_frame_service.h"     // nogncheck
#include "clay/shell/common/services/vsync_waiter_service.h"     // nogncheck

namespace clay {

// PageView references these auto-init services from methods that are not used
// by exposure tests. Keep the dedicated test target independent from a full
// platform shell by providing inert factories and callbacks.
std::shared_ptr<InstrumentationService> InstrumentationService::Create() {
  return nullptr;
}

void InstrumentationService::AddFrameTimingListener(
    std::shared_ptr<FrameTimingListener>) {}

void InstrumentationService::RemoveFrameTimingListener(
    std::shared_ptr<FrameTimingListener>) {}

std::shared_ptr<RasterFrameService> RasterFrameService::Create() {
  return nullptr;
}

void RasterFrameService::NotifyUploadTaskRegistered() {}

std::shared_ptr<VsyncWaiterService> VsyncWaiterService::Create() {
  return nullptr;
}

}  // namespace clay
