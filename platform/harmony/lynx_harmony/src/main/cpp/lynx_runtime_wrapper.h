// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_RUNTIME_WRAPPER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_RUNTIME_WRAPPER_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/lynx_actor.h"
#include "base/include/value/base_value.h"
#include "core/public/devtool/lynx_devtool_proxy.h"
#include "core/public/devtool/lynx_inspector_owner.h"
#include "core/public/lynx_extension_delegate.h"
#include "core/public/ui_delegate.h"
#include "core/public/vsync_observer_interface.h"
#include "core/renderer/data/template_data.h"
#include "core/resource/lynx_resource_loader_harmony.h"
#include "core/runtime/js/bindings/modules/harmony/module_factory_harmony.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"
#include "core/shell/native_facade_empty_implementation.h"
#include "core/shell/runtime/bts/bts_runtime_standalone_helper.h"
#include "core/shell/runtime/bts/lynx_bts_runtime_proxy_impl.h"
#include "third_party/napi/include/napi.h"

namespace lynx {

namespace harmony {
class LynxRuntimeWrapper;

class NativeRuntimeFacadeHarmony : public shell::NativeFacadeEmptyImpl {
 public:
  explicit NativeRuntimeFacadeHarmony(LynxRuntimeWrapper* runtime_wrapper)
      : runtime_wrapper_(runtime_wrapper) {}
  ~NativeRuntimeFacadeHarmony() override { runtime_wrapper_ = nullptr; }
  void ReportError(const base::LynxError& error) override;
  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t code) override;
  void OnEvaluateJavaScriptEnd(const std::string& url) override;

 private:
  LynxRuntimeWrapper* runtime_wrapper_;
};

class RuntimeLifecycleListenerDelegateHarmony
    : public runtime::RuntimeLifecycleListenerDelegate {
 public:
  explicit RuntimeLifecycleListenerDelegateHarmony(napi_env env,
                                                   napi_ref listener_ref);
  ~RuntimeLifecycleListenerDelegateHarmony() override = default;
  void OnRuntimeCreate(
      std::shared_ptr<runtime::IVSyncObserver> observer) override {}
  void OnRuntimeInit(int64_t runtime_id) override {}
  void OnAppEnterForeground() override {}
  void OnAppEnterBackground() override {}
  void OnRuntimeAttach(Napi::Env env) override;
  void OnRuntimeDetach() override;

 private:
  napi_env env_;
  napi_ref listener_ref_;
};

class LynxRuntimeWrapper : public devtool::LynxDevToolProxy {
 public:
  LynxRuntimeWrapper(
      napi_env env, napi_value js_this,
      const std::shared_ptr<LynxResourceLoaderHarmony>& resource_loader,
      std::string group_id, bool use_quickjs, bool enable_js_group_thread,
      std::vector<std::string> preload_js_paths, bool enable_bytecode,
      std::string bytecode_source_url,
      std::unique_ptr<ModuleFactoryHarmony> module_factory,
      std::shared_ptr<tasm::TemplateData> template_data,
      lepus::Value global_props);

  virtual ~LynxRuntimeWrapper();
  LynxRuntimeWrapper(const LynxRuntimeWrapper& facade) = delete;
  LynxRuntimeWrapper& operator=(const LynxRuntimeWrapper&) = delete;
  LynxRuntimeWrapper(LynxRuntimeWrapper&& facade) = delete;
  LynxRuntimeWrapper& operator=(LynxRuntimeWrapper&&) = delete;

  // LynxDevToolProxy override start
  void ReloadTemplate(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<tasm::TemplateData>& data) override {}
  void LoadTemplateFromURL(
      const std::string& url,
      const std::shared_ptr<tasm::TemplateData> data = nullptr) override {}
  double GetScreenScaleFactor() override { return 1.f; };
  void TakeSnapshot(
      size_t max_width, size_t max_height, int quality,
      float screen_scale_factor,
      const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
      tasm::TakeSnapshotCompletedCallback callback) override{};
  int GetNodeForLocation(int x, int y) override { return 0; };
  std::vector<float> GetTransformValue(
      int id, const std::vector<float>& pad_border_margin_layout) override {
    return std::vector<float>();
  };

  void SetInspectorOwner(devtool::LynxInspectorOwner* owner) override {
    inspector_owner_ = owner;
  };
  void EmulateTouch(const std::string& event_type, int x, int y,
                    const std::string& button, float delta_x, float delta_y,
                    int modifiers, int click_count) override {}
  // LynxDevToolProxy override end

  shell::BTSRuntimeStandalone& RuntimeStandalone() {
    return *runtime_standalone_;
  }

  std::shared_ptr<runtime::js::LynxModuleManager> GetModuleManager() {
    return module_manager_;
  }

  std::shared_ptr<shell::LynxRuntimeProxy> GetRuntimeProxy() {
    return runtime_proxy_;
  }

  void SetAttached(bool is_attached);
  void AddRuntimeLifecycleListener(napi_env env, napi_ref ref);

  static napi_value Init(napi_env env, napi_value exports);
  static napi_value New(napi_env env, napi_callback_info info);
  static napi_value NativeCreate(napi_env env, napi_callback_info info);
  static napi_value NativeEvaluateScript(napi_env env, napi_callback_info info);
  static napi_value NativeEvaluateTemplateBundle(napi_env env,
                                                 napi_callback_info info);
  static napi_value NativeTransitionToFullRuntime(napi_env env,
                                                  napi_callback_info info);
  static napi_value NativeCallJSFunction(napi_env env, napi_callback_info info);
  static napi_value NativeAddRuntimeLifecycleListener(napi_env env,
                                                      napi_callback_info info);

  napi_env env_;
  napi_ref runtime_wrapper_ref_;

 private:
  void DestroyRuntime();
  std::unique_ptr<shell::BTSRuntimeStandalone> runtime_standalone_;
  std::shared_ptr<shell::LynxBTSRuntimeProxyImpl> runtime_proxy_;
  // TODO(liyanbo.monster): use weak_ptr instead of shared_ptr
  std::shared_ptr<runtime::js::LynxModuleManager> module_manager_;
  bool is_attached_{false};
  devtool::LynxInspectorOwner* inspector_owner_ = nullptr;
};

}  // namespace harmony
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_RUNTIME_WRAPPER_H_
