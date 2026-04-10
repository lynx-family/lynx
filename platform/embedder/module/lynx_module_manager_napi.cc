// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/module/lynx_module_manager_napi.h"

#include "base/include/log/logging.h"
#include "core/runtime/js/runtime_lifecycle_listener_delegate.h"
#include "core/shell/runtime/bts/lynx_bts_runtime_proxy_impl.h"
#include "third_party/weak-node-api/headers/napi.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

class EmbedderRuntimeLifecycleListenerDelegate
    : public runtime::RuntimeLifecycleListenerDelegate {
 public:
  explicit EmbedderRuntimeLifecycleListenerDelegate(
      std::weak_ptr<LynxModuleManagerNAPI> module_manager)
      : RuntimeLifecycleListenerDelegate(
            RuntimeLifecycleListenerDelegate::DelegateType::FULL),
        module_manager_(std::move(module_manager)) {}
  ~EmbedderRuntimeLifecycleListenerDelegate() override = default;
  void OnRuntimeCreate(
      std::shared_ptr<runtime::IVSyncObserver> observer) override {}
  void OnRuntimeInit(int64_t runtime_id) override {}
  void OnAppEnterForeground() override {}
  void OnAppEnterBackground() override {}
  void OnRuntimeAttach(void* env) override {
    if (auto module_manager = module_manager_.lock()) {
      module_manager->OnRuntimeAttach(Napi::Env(static_cast<napi_env>(env)));
    }
  }
  void OnRuntimeDetach() override {}

 private:
  std::weak_ptr<LynxModuleManagerNAPI> module_manager_;
};

LynxModuleManagerNAPI::LynxModuleManagerNAPI(
    void* context,
    std::shared_ptr<runtime::js::LynxModuleManager> module_manager,
    std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
        module_creators)
    : view_context_(context),
      module_manager_(std::move(module_manager)),
      module_creators_(std::move(module_creators)) {}

void LynxModuleManagerNAPI::SetupRuntimeLifecycleListener(
    std::shared_ptr<shell::LynxRuntimeProxy> runtime_proxy) {
  auto runtime_proxy_impl =
      static_cast<shell::LynxBTSRuntimeProxyImpl*>(runtime_proxy.get());
  if (!runtime_proxy_impl) {
    return;
  }
  runtime_proxy_impl->AddLifecycleListener(
      std::make_unique<EmbedderRuntimeLifecycleListenerDelegate>(
          weak_from_this()));
}

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif
#include "third_party/napi/include/js_native_api.h"
void LynxModuleManagerNAPI::OnRuntimeAttach(Napi::Env env) {
  lynx_napi_set_instance_data(env, LYNX_NAPI_ENV_LYNX_VIEW_TAG, view_context_,
                              nullptr, nullptr);

  // Compatibility layer:
  // Some modules still depend on legacy instance data in the short term, so we
  // keep writing instance data via the legacy NAPI entry points when available.
  // TODO(wujintian): Remove this once all legacy instance-data users are gone.
#ifdef USE_WEAK_SUFFIX_NAPI
  LOGI("LynxModuleManagerNAPI::OnRuntimeAttach, set "
       << view_context_
       << " to legacy instance data "
          "with key "
       << LYNX_NAPI_ENV_LYNX_VIEW_TAG);
  napi_env_weak env_weak = static_cast<napi_env_weak>(env);
#ifdef USE_PRIMJS_NAPI
  napi_env_primjs env_primjs = reinterpret_cast<napi_env_primjs>(env_weak);
#else
  napi_env env_primjs = reinterpret_cast<napi_env>(env_weak);
#endif
  env_primjs->napi_set_instance_data(env_primjs, LYNX_NAPI_ENV_LYNX_VIEW_TAG,
                                     view_context_, nullptr, nullptr);
#endif

  auto module_factory =
      std::make_unique<LynxModuleFactoryNAPI>(env, std::move(module_creators_));
  module_factory_ = module_factory.get();
  module_manager_->SetModuleFactory(std::move(module_factory));
}

void LynxModuleManagerNAPI::Detach() {
  // Called from Main thread before LynxView destroyed, and should be called
  // before runtime detached.
  if (module_factory_) {
    module_factory_->Detach();
  }
}

}  // namespace embedder
}  // namespace lynx
