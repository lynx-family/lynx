// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/loader/resource_loader_factory.h"

#include <utility>

#include "clay/common/service/service_manager.h"
#include "clay/net/loader/data_image_loader.h"
#include "clay/net/loader/resource_loader_creator_service.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/net/loader/resource_loader_platform.h"
#include "clay/net/url/url_helper.h"

namespace clay {

// static
std::shared_ptr<ResourceLoader> ResourceLoaderFactory::Create(
    const std::string& uri, fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    std::shared_ptr<ServiceManager> service_manager) {
  switch (url::ParseUriScheme(uri)) {
    case url::UriSchemeType::kData:
      return std::make_shared<DataImageLoader>(std::move(task_runner));
    default: {
      std::shared_ptr<ResourceLoader> resource_loader = nullptr;
      if (service_manager) {
        auto creator_service =
            service_manager
                ->GetMultiThreadService<ResourceLoaderCreatorService>();
        // Try to create resource loader from creator service if it exists. The
        // host application can register a creator service to create resource
        // loader.
        if (creator_service) {
          resource_loader =
              creator_service->CreateResourceLoader(task_runner, intercept);
        }
      }

      if (!resource_loader) {
        // Fallback to create platform resource loader.
        resource_loader = CreatePlatformResourceLoader(
            intercept, std::move(task_runner), service_manager);
      }
      return resource_loader;
    }
  }
}
}  // namespace clay
