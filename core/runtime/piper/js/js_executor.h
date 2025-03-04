// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PIPER_JS_JS_EXECUTOR_H_
#define CORE_RUNTIME_PIPER_JS_JS_EXECUTOR_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "core/runtime/bindings/jsi/global.h"
#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_binding.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"
#include "core/runtime/jsi/jsi.h"
#include "third_party/rapidjson/document.h"
#if ENABLE_TESTBENCH_REPLAY
#include "core/services/replay/lynx_module_manager_testbench.h"
#endif

namespace lynx {

namespace runtime {
class RuntimeManager;
class TemplateDelegate;
class LynxApiHandler;
class RuntimeManagerDelegate;
}  // namespace runtime

namespace piper {

class BASE_EXPORT_FOR_DEVTOOL JSExecutor {
 public:
  JSExecutor(const std::shared_ptr<JSIExceptionHandler>& handler,
             const std::string& group_id,
             const std::shared_ptr<LynxModuleManager>& module_manager,
             const std::shared_ptr<piper::InspectorRuntimeObserverNG>&
                 runtime_observer,
             bool forceUseLightweightJSEngine = false);
  ~JSExecutor();
  JSExecutor(const JSExecutor&) = delete;
  JSExecutor& operator=(const JSExecutor&) = delete;

  void Destroy();

  void loadPreJSBundle(
      std::vector<std::pair<std::string, std::string>>& js_pre_sources,
      bool ensure_console, int64_t rt_id, bool enable_user_bytecode,
      const std::string& bytecode_source_url);

  void invokeCallback(std::shared_ptr<piper::ModuleCallback> callback,
                      piper::ModuleCallbackFunctionHolder* holder);

  runtime::RuntimeManager* runtimeManagerInstance();

  std::shared_ptr<piper::App> createNativeAppInstance(
      int64_t rt_id, runtime::TemplateDelegate*,
      std::unique_ptr<lynx::runtime::LynxApiHandler> api_handler);

  piper::JSRuntimeCreatedType getJSRuntimeType();

  std::shared_ptr<piper::Runtime> GetJSRuntime();

  void SetUrl(const std::string& url);

  std::shared_ptr<piper::ConsoleMessagePostMan> CreateConsoleMessagePostMan();

  static runtime::RuntimeManager* GetCurrentRuntimeManagerInstance();

  const std::shared_ptr<piper::InspectorRuntimeObserverNG>
  GetRuntimeObserver() {
    return runtime_observer_ng_;
  }

 protected:
  std::shared_ptr<JSIExceptionHandler> exception_handler_;
  std::string group_id_;
  std::shared_ptr<piper::InspectorRuntimeObserverNG> runtime_observer_ng_;
  std::shared_ptr<piper::LynxModuleManager> module_manager_;
  bool force_use_light_weight_js_engine_;
#if ENABLE_TESTBENCH_REPLAY
  std::shared_ptr<piper::ModuleManagerTestBench> module_manager_testBench_;
#endif

  // set by  the child class
  std::shared_ptr<piper::Runtime> js_runtime_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_PIPER_JS_JS_EXECUTOR_H_
