// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_PLATFORM_CONST_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_PLATFORM_CONST_SERVICE_H_

#include <memory>
#include <mutex>

#include "base/include/fml/synchronization/sync_switch.h"
#include "clay/common/service/service.h"
#include "clay/common/settings.h"
#include "clay/shell/common/display_manager.h"

namespace clay {

class PlatformConstService
    : public clay::Service<PlatformConstService, clay::Owner::kPlatform,
                           clay::ServiceFlags::kMultiThread |
                               clay::ServiceFlags::kManualRegister> {
 public:
  const std::shared_ptr<fml::SyncSwitch>& GetIsGPUDisabledSyncSwitch() const {
    return is_gpu_disabled_sync_switch_;
  }

  const Settings& GetSettings() const { return settings_; }

  fml::Milliseconds GetFrameBudget() const;

  explicit PlatformConstService(
      const Settings& settings, bool is_gpu_disabled,
      std::shared_ptr<DisplayManager> display_manager);

  void OnDestroy() override;

 private:
  Settings settings_;
  std::shared_ptr<fml::SyncSwitch> is_gpu_disabled_sync_switch_;

  std::shared_ptr<DisplayManager> display_manager_;
  mutable std::mutex display_manager_mutex_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_PLATFORM_CONST_SERVICE_H_
