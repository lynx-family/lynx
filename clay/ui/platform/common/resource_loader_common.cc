// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/loader/resource_loader_platform.h"

#if defined(ENABLE_NET_LOADER)
#include "clay/ui/platform/common/resource_loader_common_net.h"
#endif  // ENABLE_NET_LOADER

namespace clay {

std::shared_ptr<ResourceLoader> CreatePlatformResourceLoader(
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ServiceManager> service_manager) {
#if defined(ENABLE_NET_LOADER)
  return std::make_shared<ResourceLoaderCommon>(intercept,
                                                std::move(task_runner));
#endif
  return nullptr;
}

}  // namespace clay
