// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LOADER_H_
#define CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LOADER_H_

#include <cstdint>
#include <memory>
#include <mutex>
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
#include "core/resource/lazy_bundle/bundle_manager.h"
#include "core/resource/lazy_bundle/lazy_bundle_lifecycle_option.h"
#include "core/resource/lazy_bundle/lazy_bundle_request.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace shell {
class BTSRuntime;
class LynxEngine;

struct ExternalResourceInfo {
  std::vector<uint8_t> data;
  int32_t err_code{0};
  std::string err_msg;

  ExternalResourceInfo() = default;

  ExternalResourceInfo(std::vector<uint8_t> data, int32_t err_code,
                       std::string err_msg)
      : data(std::move(data)),
        err_code(err_code),
        err_msg(std::move(err_msg)) {}

  ExternalResourceInfo(int32_t err_code, std::string err_msg)
      : err_code(err_code), err_msg(std::move(err_msg)) {}

  bool Success() const { return err_code == 0; }
};
}  // namespace shell

namespace tasm {

// Loads template bundles and external runtime resources. A loader can dispatch
// results to both the engine and runtime actors, while its BundleManager can be
// shared with another loader when the actors have different lifecycles.
class LazyBundleLoader : public std::enable_shared_from_this<LazyBundleLoader> {
  using UrlToLifecycleOptionMap = std::unordered_map<
      std::string, std::vector<std::unique_ptr<LazyBundleLifecycleOption>>>;

 public:
  struct CallBackInfo {
    struct StatusInfo {
      StatusInfo() = delete;
      StatusInfo(int32_t status_code, std::string status_message)
          : code(status_code), message(std::move(status_message)) {}

      int32_t code;
      std::string message;
    };

    static std::optional<StatusInfo> CreateStatusInfo(int32_t code,
                                                      std::string message) {
      if (code == error::E_SUCCESS) {
        return std::nullopt;
      }
      return StatusInfo{code, std::move(message)};
    }

    CallBackInfo(std::string url, std::vector<uint8_t> data,
                 const std::optional<LynxTemplateBundle>& component_bundle,
                 std::optional<StatusInfo> status,
                 RadonLazyComponent* component, int instance_id)
        : component_url(std::move(url)),
          data(std::move(data)),
          component(component),
          instance_id_(instance_id),
          bundle(component_bundle) {
      HandleStatus(std::move(status));
    }

    // for preload
    CallBackInfo(std::string url, std::vector<uint8_t> data,
                 const std::optional<LynxTemplateBundle>& component_bundle,
                 std::optional<StatusInfo> status)
        : component_url(std::move(url)),
          data(std::move(data)),
          bundle(component_bundle) {
      HandleStatus(std::move(status));
    }

    // for js
    CallBackInfo(std::string url, std::vector<uint8_t> data,
                 const std::optional<LynxTemplateBundle>& component_bundle,
                 std::optional<StatusInfo> status, bool sync,
                 int32_t callback_id, std::vector<std::string> component_ids)
        : component_url(std::move(url)),
          data(std::move(data)),
          sync(sync),
          bundle(component_bundle),
          callback_id(callback_id),
          component_ids(std::move(component_ids)) {
      HandleStatus(std::move(status));
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
    // TODO(zhoupeng.z): all info from request should be moved to
    // LynxLazyBundleRequest
    lazy_bundle::LynxLazyBundleRequest request;

   private:
    void HandleStatus(std::optional<StatusInfo> status);
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
  LazyBundleLoader() : LazyBundleLoader(nullptr) {}
  explicit LazyBundleLoader(
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      std::shared_ptr<BundleManager> bundle_manager = nullptr)
      : resource_loader_(resource_loader),
        bundle_manager_(bundle_manager ? std::move(bundle_manager)
                                       : std::make_shared<BundleManager>()) {}

  virtual ~LazyBundleLoader() = default;
  inline void SetEngineActor(
      std::shared_ptr<shell::LynxActor<shell::LynxEngine>> actor) {
    engine_actor_ = std::move(actor);
  }
  inline void SetRuntimeActor(
      const std::shared_ptr<shell::LynxActor<shell::BTSRuntime>>& actor) {
    runtime_actor_ = actor;
  }
  inline void SetPerfControllerActor(
      std::shared_ptr<
          shell::LynxActor<tasm::performance::PerformanceController>>
          actor) {
    perf_controller_actor_ = std::move(actor);
  }

  virtual void RequireTemplate(RadonLazyComponent* lazy_bundle,
                               const std::string& url, int instance_id);

  /**
   * Load bundle for frame
   */
  void LoadFrameBundle(const std::string& src);

  void DidLoadComponent(LazyBundleLoader::CallBackInfo);

  bool RequireTemplateCollected(RadonLazyComponent* lazy_bundle,
                                const std::string& url, int instance_id);

  void MarkComponentLoading(const std::string& url);

  void AppendUrlToLifecycleOptionMap(
      const std::string& url, std::unique_ptr<LazyBundleLifecycleOption>);

  bool DispatchOnComponentLoaded(TemplateAssembler* tasm,
                                 const std::string& url);

  virtual void PreloadTemplates(const std::vector<std::string>& urls);

  /**
   * Load bundle for frame, preload and js fetching, will call DidFetchBundle.
   * TODO(zhoupeng.z): Merge with other apis about fetching bundle.
   */
  void FetchBundle(lazy_bundle::LynxLazyBundleRequest request);

  /**
   * Callback of FetchBundle, will do predecode
   */
  void DidFetchBundle(LazyBundleLoader::CallBackInfo callback_info);

  shell::ExternalResourceInfo LoadScript(const std::string& url, long timeout);

  shell::ExternalResourceInfo LoadByteCode(const std::string& url,
                                           long timeout);

  void LoadScriptAsync(const std::string& url, int32_t callback_id);

  void LoadLazyBundle(const std::string& url, int32_t callback_id);

  void LoadLazyBundle(std::string url, int32_t callback_id,
                      std::vector<std::string> component_ids);

  std::vector<uint8_t> LoadJSSource(const std::string& url);

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

  void InsertTemplateBundle(const std::string& url, LynxTemplateBundle bundle);

  std::optional<LynxTemplateBundle> GetTemplateBundle(const std::string& url);

  std::shared_ptr<BundleManager> GetBundleManager() const;

  void SetBundleManager(std::shared_ptr<BundleManager> bundle_manager);

 protected:
  virtual void ReportErrorInner(int32_t code, const std::string& msg){};

 private:
  void DidLoadComponentFromJS(CallBackInfo callback_info);

  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
  std::weak_ptr<shell::LynxActor<shell::BTSRuntime>> runtime_actor_;
  std::shared_ptr<shell::LynxActor<tasm::performance::PerformanceController>>
      perf_controller_actor_;
  std::shared_ptr<pub::LynxResourceLoader> resource_loader_ = nullptr;
  std::set<std::string> requiring_urls_{};
  UrlToLifecycleOptionMap url_to_lifecycle_option_map_{};

  friend class RequireScope;
  RadonLazyComponent* requiring_component_{nullptr};

  bool enable_component_async_decode_{false};

  std::shared_ptr<BundleManager> bundle_manager_;
  mutable std::mutex bundle_manager_mutex_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LOADER_H_
