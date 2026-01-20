// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_MODULE_LYNX_EXTENSION_MODULE_PRIV_H_
#define PLATFORM_EMBEDDER_MODULE_LYNX_EXTENSION_MODULE_PRIV_H_

#include <memory>
#include <string>

#include "core/public/jsb/lynx_extension_module.h"
#include "platform/embedder/public/capi/lynx_extension_module_capi.h"
#include "platform/embedder/public/capi/lynx_view_capi.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace embedder {
class ExtensionModuleImpl;
class LynxNativeModuleNAPI;
}  // namespace embedder
}  // namespace lynx

struct lynx_vsync_observer_t {
  lynx::embedder::ExtensionModuleImpl* extension_module = nullptr;
};

struct lynx_extension_module_t {
  void* user_data = nullptr;
  void (*finalizer)(lynx_extension_module_t*, void*) = nullptr;
  std::atomic_int ref_count = 0;
  lynx_extension_module_on_lynx_view_create_func on_lynx_view_create_func =
      nullptr;
  lynx_extension_module_on_lynx_view_destroy_func on_lynx_view_destroy_func =
      nullptr;
  lynx_extension_module_on_runtime_init_func on_runtime_init_func = nullptr;
  lynx_extension_module_on_runtime_attach_func on_runtime_attach_func = nullptr;
  lynx_extension_module_on_runtime_ready_func on_runtime_ready_func = nullptr;
  lynx_extension_module_on_runtime_detach_func on_runtime_detach_func = nullptr;
  lynx_extension_module_on_enter_foreground_func on_enter_foreground_func =
      nullptr;
  lynx_extension_module_on_enter_background_func on_enter_background_func =
      nullptr;
  lynx_extension_module_on_destroy_func on_destroy_func = nullptr;
  napi_module_creator napi_creator = nullptr;
  lynx_vsync_observer_t* vsync_observer = nullptr;
  lynx::embedder::ExtensionModuleImpl* extension_module = nullptr;
  fml::RefPtr<fml::TaskRunner> task_runner = nullptr;
};

namespace lynx {
namespace embedder {

class ExtensionModuleImpl : public runtime::LynxExtensionModule {
 public:
  ExtensionModuleImpl(lynx_extension_module_t* module);
  ~ExtensionModuleImpl();

  void SetLynxViewCreatedState(lynx_view_t* lynx_view,
                               tasm::UIDelegate* ui_delegate);
  void SetLynxViewCreatedState(tasm::UIDelegate* ui_delegate) override {}
  void SetLynxViewDestroyedState() override;

  void SetRuntimeInitState(
      const fml::RefPtr<fml::TaskRunner>& task_runner) override;
  void SetRuntimeAttachedState(
      napi_env env,
      const std::shared_ptr<runtime::IVSyncObserver>& vsync_observer) override;
  void SetRuntimeReadyState(napi_env env, napi_value lynx,
                            const std::string& url) override;
  void SetRuntimeDetachedState() override;
  void SetEnteringForegroundState() override;
  void SetEnteringBackgroundState() override;

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const runtime::CallbackMap& callbacks) override;

  void SetDelegate(
      std::weak_ptr<runtime::LynxNativeModule::Delegate> delegate) override;
  void SetupNapiModule();

  runtime::IVSyncObserver* VSyncObserver() { return vsync_observer_.get(); }

  void Destroy() override;

 private:
  lynx_extension_module_t* c_module_;
  napi_env env_;
  std::shared_ptr<runtime::IVSyncObserver> vsync_observer_;
  std::unique_ptr<LynxNativeModuleNAPI> napi_module_;
};

}  // namespace embedder
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_MODULE_LYNX_EXTENSION_MODULE_PRIV_H_
