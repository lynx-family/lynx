// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/vsync_waiter_fallback.h"

#include <algorithm>
#include <memory>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "base/include/fml/message_loop.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/shell/common/services/vsync_waiter_service.h"

namespace clay {
namespace {

static fml::TimePoint SnapToNextTick(fml::TimePoint value,
                                     fml::TimePoint tick_phase,
                                     fml::TimeDelta tick_interval) {
  fml::TimeDelta offset = (tick_phase - value) % tick_interval;
  if (offset != fml::TimeDelta::Zero()) {
    offset = offset + tick_interval;
  }
  return value + offset;
}

}  // namespace

class FallbackVsyncWaiterService : public VsyncWaiterService {
  std::unique_ptr<VsyncWaiter> CreateVsyncWaiter(
      fml::RefPtr<fml::TaskRunner> task_runner) const override {
    return std::make_unique<VsyncWaiterFallback>(std::move(task_runner));
  }
};

std::shared_ptr<VsyncWaiterService> CreateFallbackVsyncWaiterService() {
  return std::make_shared<FallbackVsyncWaiterService>();
}

#ifndef __APPLE__
std::shared_ptr<VsyncWaiterService> VsyncWaiterService::Create() {
  FML_DLOG(WARNING)
      << "This platform does not provide a Vsync waiter implementation. A "
         "simple timer based fallback is being used.";
  return CreateFallbackVsyncWaiterService();
}
#endif

VsyncWaiterFallback::VsyncWaiterFallback(
    fml::RefPtr<fml::TaskRunner> task_runner)
    : VsyncWaiter(std::move(task_runner)), phase_(fml::TimePoint::Now()) {}

VsyncWaiterFallback::~VsyncWaiterFallback() = default;

// |VsyncWaiter|
void VsyncWaiterFallback::AwaitVSync() {
  static fml::TimeDelta kSingleFrameInterval =
      fml::TimeDelta::FromSecondsF(1.0 / GetRefreshRate());
  auto frame_start_time =
      SnapToNextTick(fml::TimePoint::Now(), phase_, kSingleFrameInterval);
  auto frame_target_time = frame_start_time + kSingleFrameInterval;

  std::weak_ptr<VsyncWaiterFallback> weak_this =
      std::static_pointer_cast<VsyncWaiterFallback>(shared_from_this());

  task_runner_->PostTaskForTime(
      [frame_start_time, frame_target_time, weak_this]() {
        if (auto vsync_waiter = weak_this.lock()) {
          vsync_waiter->FireCallback(frame_start_time, frame_target_time);
        }
      },
      frame_start_time);
}

double VsyncWaiterFallback::GetRefreshRate() const {
  int rate = VsyncWaiter::GetRefreshRate();
#ifdef _WIN32
  HDC hdc = ::GetDC(NULL);
  if (hdc != NULL) {
    rate = std::max(rate, ::GetDeviceCaps(hdc, VREFRESH));
    ::ReleaseDC(NULL, hdc);
  }
#endif
  return rate;
}

}  // namespace clay
