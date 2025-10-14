// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_LOADER_RESOURCE_LOADER_CREATOR_SERVICE_H_
#define CLAY_NET_LOADER_RESOURCE_LOADER_CREATOR_SERVICE_H_

#include <memory>

#include "clay/common/service/service.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_intercept.h"

namespace clay {

using ResourceLoaderCreator = std::function<std::shared_ptr<ResourceLoader>(
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ResourceLoaderIntercept> intercept)>;

class ResourceLoaderCreatorService
    : public Service<ResourceLoaderCreatorService, Owner::kPlatform,
                     ServiceFlags::kManualRegister |
                         ServiceFlags::kMultiThread> {
 public:
  explicit ResourceLoaderCreatorService(ResourceLoaderCreator creator)
      : creator_(creator) {}
  ~ResourceLoaderCreatorService() override = default;

  std::shared_ptr<ResourceLoader> CreateResourceLoader(
      fml::RefPtr<fml::TaskRunner> task_runner,
      std::shared_ptr<ResourceLoaderIntercept> intercept) {
    if (!creator_) {
      return nullptr;
    }
    return creator_(task_runner, intercept);
  }

 private:
  ResourceLoaderCreator creator_;
};
}  // namespace clay

#endif  // CLAY_NET_LOADER_RESOURCE_LOADER_CREATOR_SERVICE_H_
