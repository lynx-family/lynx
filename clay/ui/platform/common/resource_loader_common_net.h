// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_COMMON_RESOURCE_LOADER_COMMON_NET_H_
#define CLAY_UI_PLATFORM_COMMON_RESOURCE_LOADER_COMMON_NET_H_

#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "clay/net/loader/resource_loader.h"
#include "clay/net/net_loader_manager.h"
#include "clay/net/resource_type.h"

namespace clay {

struct RawResource;
class ResourceLoaderIntercept;
class ResourceLoaderCommon
    : public ResourceLoader,
      public std::enable_shared_from_this<ResourceLoaderCommon> {
 public:
  explicit ResourceLoaderCommon(
      std::shared_ptr<ResourceLoaderIntercept> intercept,
      fml::RefPtr<fml::TaskRunner> task_runner);
  ~ResourceLoaderCommon() override;

  void Load(const std::string& src,
            const std::function<void(const uint8_t*, size_t)>& callback,
            const ResourceType resource_type = ResourceType::kOthers,
            bool need_redirect = false) override;

  RawResource LoadSync(const std::string& src,
                       const ResourceType resource_type = ResourceType::kOthers,
                       bool need_redirect = false) override;

  void CancelAll() override;

  bool NeedInterceptUrl() override { return true; }

 private:
  void RealLoad(std::weak_ptr<ResourceLoaderCommon> weak_ref,
                fml::RefPtr<fml::TaskRunner> ui_task_runner,
                const std::string& src,
                const std::function<void(const uint8_t*, size_t)>& callback);
  void LoadOnNet(std::weak_ptr<ResourceLoaderCommon> weak_ref,
                 fml::RefPtr<fml::TaskRunner> ui_task_runner,
                 const std::string& src,
                 const std::function<void(const uint8_t*, size_t)>& callback);
  void LoadByFile(fml::RefPtr<fml::TaskRunner> ui_task_runner,
                  const std::string& src,
                  const std::function<void(const uint8_t*, size_t)>& callback);
  void OnLoadFinished(size_t request_seq);

  std::set<size_t> pending_requests_;
  std::mutex pending_requests_mutex_;

  RawResource resource_;

  std::shared_ptr<ResourceLoaderIntercept> resource_loader_intercept_;
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
};
}  // namespace clay

#endif  // CLAY_UI_PLATFORM_COMMON_RESOURCE_LOADER_COMMON_NET_H_
