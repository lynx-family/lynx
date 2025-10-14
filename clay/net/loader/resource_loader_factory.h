// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_LOADER_RESOURCE_LOADER_FACTORY_H_
#define CLAY_NET_LOADER_RESOURCE_LOADER_FACTORY_H_

#include <memory>
#include <string>

#include "base/include/fml/task_runner.h"

namespace clay {

class ResourceLoader;
class ResourceLoaderIntercept;
class ServiceManager;

// A factory use to create Platform |ResourceLoader|.
// ResourceLoader->load(src,callback) will post task to task_runner expect
// network task.Thus callback will execute in task_runner.
//
// If we making a network request, net module will use its network thread
// to initiate network requests.task_runner is not use in Asynchronous network
// request.After network Request completion,ResourceLoader will use task_runner
// to execute callback.
//
// So we can assume that asynchronous tasks will execute in task_runner and
// callback will be called in task_runner in any cases.
class ResourceLoaderFactory {
 public:
  static std::shared_ptr<ResourceLoader> Create(
      const std::string& uri, fml::RefPtr<fml::TaskRunner> task_runner,
      std::shared_ptr<ResourceLoaderIntercept> intercept = nullptr,
      std::shared_ptr<ServiceManager> service_manager = nullptr);
};

}  // namespace clay

#endif  // CLAY_NET_LOADER_RESOURCE_LOADER_FACTORY_H_
