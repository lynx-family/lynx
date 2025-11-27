// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/common/clay_service_manager_service_darwin.h"

namespace clay {

ClayServiceManagerServiceDarwin::ClayServiceManagerServiceDarwin(ClayServiceManager* manager)
    : service_manager_(manager) {}

ClayServiceManager* ClayServiceManagerServiceDarwin::GetClayServiceManager() {
  return service_manager_;
}

}  // namespace clay
