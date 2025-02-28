// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LOADER_H_
#define CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LOADER_H_

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/lynx_actor.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/public/lynx_resource_loader.h"
#include "core/renderer/dom/vdom/radon/radon_lazy_component.h"
#include "core/resource/lazy_bundle/lazy_bundle_lifecycle_option.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace shell {
class LynxEngine;
}

namespace tasm {

class LazyBundleLoader : public std::enable_shared_from_this<LazyBundleLoader> {
  using UrlToLifecycleOptionMap = std::unordered_map<
      std::string, std::vector<std::unique_ptr<LazyBundleLifecycleOption>>>;

 public:
  struct CallBackInfo {
    CallBackInfo(std::string url, std::vector<uint8_t> data,
                 const std::optional<LynxTemplateBundle>& component_bundle,
                 const std::optional<std::string>& error,
                 RadonLazyComponent* component, int instance_id)
        : component_url(std::move(url)),
          data(std::move(data)),
          component(component),
          instance_id_(instance_id),
          bundle(component_bundle) {
      HandleError(error);
    }

    // for preload
    CallBackInfo(std::string url, std::vector<uint8_t> data,
                 const std::optional<LynxTemplateBundle>& component_bundle,
                 const std::optional<std::string>& error)
        : component_url(std::move(url)),
          data(std::move(data)),
          bundle(component_bundle) {
      HandleError(error);
    }

    // for js
    CallBackInfo(std::string url, std::vector<uint8_t> data,
                 const std::optional<LynxTemplateBundle>& component_bundle,
                 const std::optional<std::string>& error, bool sync,
                 int32_t callback_id, std::vector<std::string> component_ids)
        : component_url(std::move(url)),
          data(std::move(data)),
          sync(sync),
          bundle(component_bundle),
          callback_id(callback_id),
          component_ids(std::move(component_ids)) {
      HandleError(error);
    }

    CallBackInfo(const CallBackInfo&) = delete;
    CallBackInfo& operator=(const CallBackInfo&) = delete;
    CallBackInfo(CallBackInfo&&) = default;
    CallBackInfo& operator=(CallBackInfo&&) = default;
    ~CallBackInfo() = default;

    bool Success() const { return error::E_SUCCESS == error_code; }

    size_t SourceSize() const { return bundle ? bundle->Size() : data.size(); }

    std::string component_url;
    mutable std::vector<uint8_t> data;
    RadonLazyComponent* component{nullptr};
    int instance_id_{0};
    int32_t error_code{error::E_SUCCESS};
    std::string error_msg{};
    bool sync{false};
    std::optional<LynxTemplateBundle> bundle = std::nullopt;
    // for js
    int32_t callback_id{-1};
    std::vector<std::string> component_ids;

   private:
    void HandleError(const std::optional<std::string>& error);
  };

  class RequireScope {
   public:
    RequireScope(const std::shared_ptr<LazyBundleLoader>& loader,
                 RadonLazyComponent* component)
        : loader_(loader.get()) {
      loader_->requiring_component_ = component;
    }
    ~RequireScope() { loader_->requiring_component_ = nullptr; }

    RequireScope(const RequireScope&) = delete;
    RequireScope& operator=(const RequireScope&) = delete;
    RequireScope(RequireScope&&) = delete;
    RequireScope& operator=(RequireScope&&) = delete;

   private:
    LazyBundleLoader* loader_{nullptr};
  };

 public:
  LazyBundleLoader() : engine_actor_(nullptr) {}
  explicit LazyBundleLoader(
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader)
      : engine_actor_(nullptr), resource_loader_(resource_loader) {}

  virtual ~LazyBundleLoader() = default;
  inline void SetEngineActor(
      std::shared_ptr<shell::LynxActor<shell::LynxEngine>> actor) {
    engine_actor_ = actor;
  }

  virtual void RequireTemplate(RadonLazyComponent* lazy_bundle,
                               const std::string& url, int instance_id);

  void DidLoadComponent(LazyBundleLoader::CallBackInfo);

  bool RequireTemplateCollected(RadonLazyComponent* lazy_bundle,
                                const std::string& url, int instance_id);

  void MarkComponentLoading(const std::string& url);

  void AppendUrlToLifecycleOptionMap(
      const std::string& url, std::unique_ptr<LazyBundleLifecycleOption>);

  bool DispatchOnComponentLoaded(TemplateAssembler* tasm,
                                 const std::string& url);

  virtual void SetEnableLynxResourceService(bool enable) {
    if (resource_loader_) {
      resource_loader_->SetEnableLynxResourceService(enable);
    }
  }

  virtual void PreloadTemplates(const std::vector<std::string>& urls);

  void DidPreloadTemplate(LazyBundleLoader::CallBackInfo callback_info);

  // is being required synchronously
  bool SyncRequiring(const std::string& url);

  inline RadonLazyComponent* GetRequiringComponent() const {
    return requiring_component_;
  }

  // for perf.
  void StartRecordRequireTime(const std::string& url);
  void EndRecordRequireTime(const CallBackInfo& callback_info);
  void StartRecordDecodeTime(const std::string& url);
  void EndRecordDecodeTime(const std::string& url);

  // for status.
  void MarkComponentLoadedFailed(const std::string& url, int32_t error_code,
                                 const lepus::Value& error_msg);
  void MarkComponentLoadedSuccess(const std::string& url,
                                  const lepus::Value& success_msg);

  void SetEnableComponentAsyncDecode(bool enable) {
    enable_component_async_decode_ = enable;
  }

  lepus::Value GetPerfInfo(const std::string& url);

 protected:
  virtual void ReportErrorInner(int32_t code, const std::string& msg){};

 private:
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
  std::shared_ptr<pub::LynxResourceLoader> resource_loader_ = nullptr;

  std::set<std::string> requiring_urls_{};
  UrlToLifecycleOptionMap url_to_lifecycle_option_map_{};

  friend class RequireScope;
  RadonLazyComponent* requiring_component_{nullptr};

  bool enable_component_async_decode_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LOADER_H_
