// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/platform_const_service.h"

#include <utility>

namespace clay {

PlatformConstService::PlatformConstService(
    const Settings& settings, bool is_gpu_disabled,
    std::shared_ptr<DisplayManager> display_manager)
    : settings_(settings),
      is_gpu_disabled_sync_switch_(
          std::make_shared<fml::SyncSwitch>(is_gpu_disabled)),
      display_manager_(std::move(display_manager)) {}

fml::Milliseconds PlatformConstService::GetFrameBudget() const {
  std::scoped_lock<std::mutex> lock(display_manager_mutex_);
  if (!display_manager_) {
    return fml::kDefaultFrameBudget;
  }

  double display_refresh_rate = display_manager_->GetMainDisplayRefreshRate();
  if (display_refresh_rate > 0) {
    return fml::RefreshRateToFrameBudget(display_refresh_rate);
  } else {
    return fml::kDefaultFrameBudget;
  }
}

// display_manager_ should be destroyed on platform thread.
void PlatformConstService::OnDestroy() {
  std::scoped_lock<std::mutex> lock(display_manager_mutex_);
  display_manager_.reset();
}

}  // namespace clay
