// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_RESOURCE_LOADER_EMBEDDER_H_
#define CLAY_LYNX_ADAPTOR_RESOURCE_LOADER_EMBEDDER_H_

#include <memory>
#include <string>

#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "platform/embedder/fetcher/lynx_resource_fetcher_holder.h"

namespace clay {

class ResourceLoaderEmbedder : public ResourceLoader {
 public:
  ResourceLoaderEmbedder(
      fml::RefPtr<fml::TaskRunner> task_runner,
      std::shared_ptr<ResourceLoaderIntercept> intercept,
      std::shared_ptr<lynx::embedder::LynxResourceFetcherHolder>
          fetcher_holder);
  virtual ~ResourceLoaderEmbedder() = default;

  void Load(const std::string& src,
            const std::function<void(const uint8_t*, size_t)>& callback,
            const ResourceType resource_type = ResourceType::kOthers,
            bool need_redirect = false) override;

  RawResource LoadSync(const std::string& src,
                       const ResourceType = ResourceType::kOthers,
                       bool need_redirect = false) override;

 private:
  void LoadByNet(const std::string& url,
                 const std::function<void(const uint8_t*, size_t)>& callback,
                 const ResourceType resource_type);
  RawResource LoadSyncByNet(const std::string& url,
                            const ResourceType resource_type);
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
  std::shared_ptr<ResourceLoaderIntercept> intercept_;
  std::shared_ptr<lynx::embedder::LynxResourceFetcherHolder> fetcher_holder_;
};

}  // namespace clay

#endif  // CLAY_LYNX_ADAPTOR_RESOURCE_LOADER_EMBEDDER_H_
