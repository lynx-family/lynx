// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_RUNTIME_BTS_BTS_RUNTIME_STANDALONE_HELPER_H_
#define CORE_SHELL_RUNTIME_BTS_BTS_RUNTIME_STANDALONE_HELPER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/lynx_actor.h"
#include "core/inspector/observer/inspector_runtime_observer_ng.h"
#include "core/public/lynx_resource_loader.h"
#include "core/public/page_options.h"
#include "core/renderer/js_bundle_holder_impl.h"
#include "core/resource/external_resource/external_resource_loader.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"
#include "core/runtime/js/js_bundle_holder.h"
#include "core/services/performance/performance_controller.h"
#include "core/shared_data/white_board_runtime_delegate.h"
#include "core/shell/native_facade.h"

namespace lynx {
namespace shell {

class BTSRuntimeStandalone {
 public:
  class StandaloneBundleProxy : public tasm::JsBundleHolderImpl::BundleProxy {
   public:
    explicit StandaloneBundleProxy(
        const std::shared_ptr<tasm::LazyBundleLoader>& lazy_bundle_loader)
        : lazy_bundle_loader_(lazy_bundle_loader) {}
    ~StandaloneBundleProxy() override = default;

    lepus::Value GetCustomSection(const std::string& url) override {
      if (!lazy_bundle_loader_) {
        return lepus::Value();
      }
      auto bundle = lazy_bundle_loader_->GetTemplateBundle(url);
      return bundle ? bundle->GetCustomSections() : lepus::Value();
    }

   private:
    std::shared_ptr<tasm::LazyBundleLoader> lazy_bundle_loader_;
  };

  static std::unique_ptr<BTSRuntimeStandalone> InitRuntimeStandalone(
      const std::string& group_name, const std::string& group_id,
      std::unique_ptr<NativeFacade> native_facade_runtime,
      const std::shared_ptr<piper::InspectorRuntimeObserverNG>&
          runtime_observer,
      const std::shared_ptr<lynx::pub::LynxResourceLoader>& resource_loader,
      const std::shared_ptr<lynx::pub::LynxNativeModuleManager>& module_manager,
      const std::shared_ptr<tasm::PropBundleCreator>& prop_bundle_creator,
      const std::shared_ptr<tasm::WhiteBoard>& white_board,
      const std::function<
          void(const std::shared_ptr<LynxActor<BTSRuntime>>&,
               const std::shared_ptr<LynxActor<NativeFacade>>&)>&
          on_runtime_actor_created,
      std::vector<std::string> preload_js_paths,
      const std::string& bytecode_source_url, uint32_t runtime_flag,
      const lepus::Value* global_props = nullptr, bool debuggable = false,
      bool long_task_monitor_disabled = false);

  ~BTSRuntimeStandalone() = default;
  BTSRuntimeStandalone& operator=(const BTSRuntimeStandalone&) = delete;
  BTSRuntimeStandalone& operator=(BTSRuntimeStandalone&&) = delete;

  void SetPresetData(lepus::Value data);

  void EvaluateScript(std::string url, std::string script);

  void EvaluateScript(std::string url, lynx::tasm::LynxTemplateBundle* bundle,
                      std::string js_file);

  void DestroyRuntime();

  void SetSessionStorageItem(const std::string& key,
                             const std::shared_ptr<tasm::TemplateData>& data);

  void SetSessionStorageItem(const std::string& key, const lepus::Value value);
  void GetSessionStorageItem(const std::string& key,
                             std::unique_ptr<PlatformCallBack> callback);

  double SubscribeSessionStorage(const std::string& key,
                                 std::unique_ptr<PlatformCallBack> callback);

  void UnSubscribeSessionStorage(const std::string& key, double callback_id);

  const std::shared_ptr<LynxActor<BTSRuntime>>& GetRuntimeActor() {
    return runtime_actor_;
  }

  const std::shared_ptr<LynxActor<tasm::performance::PerformanceController>>&
  GetPerfControllerActor() {
    return perf_controller_actor_;
  }

  void TransitionToFullRuntime();

  int32_t GetRuntimeId() const { return runtime_id_; }

  const std::string& GroupName() const { return group_name_; }

 private:
  BTSRuntimeStandalone(
      std::string group_name, int32_t runtime_id,
      std::shared_ptr<LynxActor<BTSRuntime>> runtime_actor,
      std::shared_ptr<LynxActor<tasm::performance::PerformanceController>>
          perf_controller_actor,
      std::shared_ptr<LynxActor<NativeFacade>> native_runtime_facade,
      std::shared_ptr<tasm::WhiteBoardRuntimeDelegate> white_board_delegate,
      std::shared_ptr<tasm::LazyBundleLoader> lazy_bundle_loader,
      std::unique_ptr<StandaloneBundleProxy> js_bundle_proxy,
      std::shared_ptr<tasm::JsBundleHolderImpl> js_bundle_holder)
      : runtime_id_(runtime_id),
        runtime_actor_(runtime_actor),
        perf_controller_actor_(perf_controller_actor),
        native_runtime_facade_(native_runtime_facade),
        white_board_delegate_(white_board_delegate),
        lazy_bundle_loader_(std::move(lazy_bundle_loader)),
        js_bundle_proxy_(std::move(js_bundle_proxy)),
        js_bundle_holder_(std::move(js_bundle_holder)) {}

  std::string group_name_;
  int32_t runtime_id_;
  std::shared_ptr<LynxActor<BTSRuntime>> runtime_actor_;
  // will be bind to LynxShell when LynxBackgroundRuntime is attached to
  // LynxView
  std::shared_ptr<LynxActor<tasm::performance::PerformanceController>>
      perf_controller_actor_;
  // will be released by LynxBackgroundRuntime if not attached to LynxView
  std::shared_ptr<LynxActor<NativeFacade>> native_runtime_facade_;
  std::shared_ptr<tasm::WhiteBoardRuntimeDelegate> white_board_delegate_;
  std::shared_ptr<tasm::LazyBundleLoader> lazy_bundle_loader_;
  std::unique_ptr<StandaloneBundleProxy> js_bundle_proxy_;
  std::shared_ptr<tasm::JsBundleHolderImpl> js_bundle_holder_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_RUNTIME_BTS_BTS_RUNTIME_STANDALONE_HELPER_H_
