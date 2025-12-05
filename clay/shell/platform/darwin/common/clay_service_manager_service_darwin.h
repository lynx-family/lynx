// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_DARWIN_COMMON_CLAY_SERVICE_MANAGER_SERVICE_DARWIN_H_
#define CLAY_SHELL_PLATFORM_DARWIN_COMMON_CLAY_SERVICE_MANAGER_SERVICE_DARWIN_H_

#import "ClayServiceManager.h"
#include "clay/common/service/service.h"

namespace clay {
class ClayServiceManagerServiceDarwin final
    : public Service<ClayServiceManagerServiceDarwin, Owner::kPlatform,
                     clay::ServiceFlags::kManualRegister |
                         ServiceFlags::kMultiThread> {
 public:
  explicit ClayServiceManagerServiceDarwin(ClayServiceManager* manager);

  ClayServiceManager* GetClayServiceManager();

 private:
  ClayServiceManager* service_manager_;
};
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_DARWIN_COMMON_SERVICES_CLAY_SERVICE_MANAGER_SERVICE_DARWIN_H_
