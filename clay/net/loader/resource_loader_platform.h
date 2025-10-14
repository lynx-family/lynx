// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_LOADER_RESOURCE_LOADER_PLATFORM_H_
#define CLAY_NET_LOADER_RESOURCE_LOADER_PLATFORM_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "clay/common/service/service_manager.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/net/resource_type.h"

namespace clay {

std::shared_ptr<ResourceLoader> CreatePlatformResourceLoader(
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ServiceManager> service_manager);

}

#endif  // CLAY_NET_LOADER_RESOURCE_LOADER_PLATFORM_H_
