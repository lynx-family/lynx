// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_VSYNC_WAITER_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_VSYNC_WAITER_SERVICE_H_

#include <memory>

#include "clay/common/service/service.h"

namespace clay {

class VsyncWaiter;

// Though vsync waiter is a platform service, it should be safe to
// create vsync waiter in any thread
class VsyncWaiterService
    : public clay::Service<VsyncWaiterService, clay::Owner::kPlatform,
                           clay::ServiceFlags::kMultiThread> {
 public:
  static std::shared_ptr<VsyncWaiterService> Create();

  virtual std::unique_ptr<VsyncWaiter> CreateVsyncWaiter(
      fml::RefPtr<fml::TaskRunner> task_runner) const = 0;

  virtual double GetRefreshRate() const { return 60.0; }
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_VSYNC_WAITER_SERVICE_H_
