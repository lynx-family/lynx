// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "clay/fml/mapping.h"
#include "clay/fml/paths.h"
#include "clay/net/loader/resource_loader_platform.h"

namespace clay {
namespace {

class TestFileResourceLoader : public ResourceLoader {
 public:
  explicit TestFileResourceLoader(fml::RefPtr<fml::TaskRunner> task_runner)
      : task_runner_(std::move(task_runner)) {}

  void Load(const std::string& src,
            const std::function<void(const uint8_t*, size_t)>& callback,
            ResourceType resource_type, bool need_redirect) override {
    if (!callback) {
      return;
    }
    auto load = [src, callback]() {
      auto mapping = fml::FileMapping::CreateReadOnly(fml::paths::FromURI(src));
      if (!mapping || !mapping->IsValid()) {
        callback(nullptr, 0);
        return;
      }
      callback(mapping->GetMapping(), mapping->GetSize());
    };
    if (task_runner_) {
      task_runner_->PostTask(std::move(load));
    } else {
      load();
    }
  }

  RawResource LoadSync(const std::string& src, ResourceType resource_type,
                       bool need_redirect) override {
    auto mapping = fml::FileMapping::CreateReadOnly(fml::paths::FromURI(src));
    if (!mapping || !mapping->IsValid()) {
      return {0, nullptr};
    }
    return RawResource::MakeWithCopy(mapping->GetMapping(), mapping->GetSize());
  }

 private:
  fml::RefPtr<fml::TaskRunner> task_runner_;
};

}  // namespace

std::shared_ptr<ResourceLoader> CreatePlatformResourceLoader(
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ServiceManager> service_manager) {
  return std::make_shared<TestFileResourceLoader>(std::move(task_runner));
}

}  // namespace clay
