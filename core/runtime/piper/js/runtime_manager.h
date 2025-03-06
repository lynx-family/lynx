// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PIPER_JS_RUNTIME_MANAGER_H_
#define CORE_RUNTIME_PIPER_JS_RUNTIME_MANAGER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/js_context_wrapper.h"

namespace lynx {

namespace piper {
class JSExecutor;
}  // namespace piper

namespace runtime {

class BASE_EXPORT_FOR_DEVTOOL RuntimeManagerDelegate {
 public:
  using ReleaseContextCallback =
      std::function<void(const std::string& group_str)>;
  using ReleaseVMCallback = std::function<void()>;

  virtual ~RuntimeManagerDelegate() = default;

  virtual void BeforeRuntimeCreate(bool force_use_lightweight_js_engine) = 0;
  virtual void OnRuntimeReady(piper::JSExecutor& executor,
                              std::shared_ptr<piper::Runtime>& current_runtime,
                              const std::string& group_id) = 0;
  virtual void AfterSharedContextCreate(const std::string& group_id,
                                        piper::JSRuntimeType type) = 0;
  virtual void OnRelease(const std::string& group_id) = 0;
  // In production environment, we use the parameter
  // "force_use_lightweight_js_engine" to determine the type of the Runtime
  // needs to be created.
  // Previously, LynxDevtool use the "debug_type_" gets from
  // InspectorJavaScriptDebugger to determine the type. After refactoring,
  // LynxDevtool will use the switch "enable_v8" together with this parameter to
  // determine the type.
  virtual std::shared_ptr<piper::Runtime> MakeRuntime(
      bool force_use_lightweight_js_engine) = 0;
#if ENABLE_TRACE_PERFETTO
  virtual std::shared_ptr<profile::RuntimeProfiler> MakeRuntimeProfiler(
      std::shared_ptr<piper::JSIContext> js_context,
      bool force_use_lightweight_js_engine) = 0;
#endif

  virtual void SetReleaseContextCallback(
      piper::JSRuntimeType type, const ReleaseContextCallback& callback) {}
  virtual void SetReleaseVMCallback(piper::JSRuntimeType type,
                                    const ReleaseVMCallback& callback) {}
};

class BASE_EXPORT_FOR_DEVTOOL RuntimeManager
    : public SharedJSContextWrapper::ReleaseListener {
 public:
  static RuntimeManager* Instance();
  typedef std::unordered_map<std::string, std::shared_ptr<JSContextWrapper>>
      Shared_Context_Map;
  typedef std::vector<std::shared_ptr<NoneSharedJSContextWrapper>>
      None_Shared_Context_List;

  RuntimeManager();
  ~RuntimeManager() override;

  bool IsSingleJSContext(const std::string& group_id);

  std::shared_ptr<piper::Runtime> CreateJSRuntime(
      const std::string& group_id,
      std::shared_ptr<piper::JSIExceptionHandler> exception_handler,
      std::vector<std::pair<std::string, std::string>>& js_pre_sources,
      bool forceUseLightweightJSEngine, piper::JSExecutor& executor,
      int64_t rt_id, bool ensure_console, bool enable_bytecode,
      const std::string& bytecode_source_url);

  void OnRelease(const std::string& group_id) override;

  RuntimeManagerDelegate* GetRuntimeManagerDelegate() {
    return runtime_manager_delegate_.get();
  }

  void SetRuntimeManagerDelegate(
      std::unique_ptr<RuntimeManagerDelegate> runtime_manager_delegate) {
    runtime_manager_delegate_ = std::move(runtime_manager_delegate);
  }

 private:
  std::shared_ptr<piper::Runtime> CreateRuntime(
      const std::string& group_id,
      std::shared_ptr<piper::JSIExceptionHandler> exception_handler,
      bool force_use_lightweight_js_engine, int64_t rt_id, bool enable_bytecode,
      const std::string& bytecode_source_url);

  std::shared_ptr<piper::JSIContext> GetSharedJSContext(
      const std::string& group_id);

  std::shared_ptr<piper::JSIContext> CreateJSIContext(
      std::shared_ptr<piper::Runtime>& rt, const std::string& group_id);

  std::shared_ptr<piper::Runtime> MakeRuntime(
      bool force_use_lightweight_js_engine);
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<profile::RuntimeProfiler> MakeRuntimeProfiler(
      std::shared_ptr<piper::JSIContext> js_context,
      bool force_use_lightweight_js_engine);
#endif

  bool EnsureVM(std::shared_ptr<piper::Runtime>& rt);
  void EnsureConsolePostMan(std::shared_ptr<piper::JSIContext>& context,
                            piper::JSExecutor& executor,
                            bool force_use_lightweight_js_engine);

  void InitJSRuntimeCreatedType(bool need_create_vm,
                                std::shared_ptr<piper::Runtime>& rt);

  bool IsInspectEnabled(bool force_use_lightweight_js_engine);

  Shared_Context_Map shared_context_map_;
  std::unordered_map<piper::JSRuntimeType, std::shared_ptr<piper::VMInstance>>
      mVMContainer_;
  std::unique_ptr<RuntimeManagerDelegate> runtime_manager_delegate_;
};

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_PIPER_JS_RUNTIME_MANAGER_H_
