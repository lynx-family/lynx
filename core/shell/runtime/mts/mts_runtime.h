// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_H_
#define CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_H_

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "base/include/fml/task_runner.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/path_parser.h"
#include "base/include/vector.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/inspector/lepus_inspector_manager.h"
#include "core/inspector/observer/inspector_lepus_observer.h"
#include "core/public/page_options.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/lepus_global.h"
#include "core/runtime/mts_context.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace tasm {
class AnimationFrameManager;
class LepusCallbackManager;
}  // namespace tasm

namespace lepus {
class QuickContext;
class VMContext;
}  // namespace lepus

namespace runtime {
class ContextBundle;

#define LEPUS_DEFAULT_CONTEXT_NAME "__Card__"

class MTSContextHolder {
 protected:
  explicit MTSContextHolder(std::unique_ptr<MTSContext> mts_context);
  ~MTSContextHolder() = default;

  std::unique_ptr<MTSContext> mts_context_;
};

class MTSRuntime : private MTSContextHolder {
 public:
  class Delegate {
   public:
    virtual const std::string& TargetSdkVersion() = 0;
    virtual void ReportError(base::LynxError error) = 0;
    virtual void OnBTSConsoleEvent(const std::string& func_name,
                                   const std::string& args) = 0;
    virtual void ReportGCTimingEvent(const char* start, const char* end) = 0;
    virtual void OnRuntimeGC(
        std::unordered_map<std::string, std::string> mem_info) = 0;

    virtual fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() = 0;

    virtual void OnScriptingStart() = 0;
    virtual void OnScriptingEnd() = 0;
  };

  class ScriptingScope {
   public:
    ScriptingScope(MTSRuntime* context);
    ~ScriptingScope();

   private:
    void CheckOnScriptingStart();
    void CheckOnScriptingEnd();

    MTSRuntime* ctx_;
  };

  virtual ~MTSRuntime();

  MTSRuntime(ContextType type, bool disable_tracing_gc = false,
             int runtime_mode = 0,
             const tasm::PageOptions& page_options = tasm::PageOptions());

  static lepus::VMContext* ToVMContext(MTSRuntime* context);

  static lepus::QuickContext* ToQuickContext(MTSRuntime* context);

  static MTSRuntime* ToContext(MTSContext* mts_context);

  static std::shared_ptr<MTSRuntime> CreateContext(
      ContextType type, bool disable_tracing_gc = false, int runtime_mode = 0,
      const tasm::PageOptions& page_options = tasm::PageOptions());

  Delegate* GetDelegate();

  // virtual interface
  void Initialize();

  // TODO(wangboyong): Remove this method after the integration is complete.
  MTSContext* GetMTSContext() { return mts_context_.get(); }

  // TODO(songshourui.null): For the MTSRuntime class, only the following
  // interfaces need to be exposed: EvalBuf, EvalBinary CallArgs, CallClosure.
  // The Execute API should not be exposed and will be deprecated in future
  // updates. Currently, EvalBinary and Execute share overlapping logic. We have
  // temporarily refactored this into a shared ExecuteBinaryInternal function.
  // Once the Execute API is removed, only EvalBinary will remain as the unified
  // entry point.
  // Execute with optional bundle info (e.g. bundleId for RTSNative).
  bool Execute(const ContextBundle* bundle);

  // only for main bundle
  bool TryExecute();

  bool HasPreExecuteSuccess();
  void ResetPreExecuteSuccess();

  void UpdateGCTiming(bool is_start);

  void TriggerVmGC();

  virtual void RegisterGlobalFunction(const RenderBindingFunction* funcs,
                                      size_t size);
  virtual void RegisterObjectFunction(lepus::Value& obj,
                                      const RenderBindingFunction* funcs,
                                      size_t size);

  bool UpdateTopLevelVariable(const std::string& name, const Value& val);
  bool UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                    const Value& val);
  // shadow equal for table
  bool CheckTableShadowUpdatedWithTopLevelVariable(const lepus::Value& update);

  void ResetTopLevelVariable();
  void ResetTopLevelVariableByVal(const Value& val);

  Value CallArgs(const base::String& name, const std::vector<Value>& args,
                 bool pause_suppression_mode = false);

  template <class... Args,
            class = std::enable_if_t<
                (std::is_same_v<
                     Value, std::remove_cv_t<std::remove_reference_t<Args>>> &&
                 ...)>>
  Value Call(const base::String& name, const Args&... args) {
    constexpr auto n_args = sizeof...(args);
    const Value* p_args[n_args] = {&args...};

    ScriptingScope scope(this);
    return mts_context_->CallArgs(name, p_args, n_args, false);
  }

  template <class... Args,
            class = std::enable_if_t<
                (std::is_same_v<
                     Value, std::remove_cv_t<std::remove_reference_t<Args>>> &&
                 ...)>>
  Value CallInPauseSuppressionMode(const base::String& name, Args&&... args) {
    constexpr auto n_args = sizeof...(args);
    const Value* p_args[n_args] = {&args...};
    return mts_context_->CallArgs(name, p_args, n_args, true);
  }

  virtual Value CallClosureArgs(const Value& closure,
                                const std::vector<Value>& args);

  template <class... Args,
            class = std::enable_if_t<
                (std::is_same_v<
                     Value, std::remove_cv_t<std::remove_reference_t<Args>>> &&
                 ...)>>
  Value CallClosure(const Value& closure, Args&&... args) {
    constexpr auto n_args = sizeof...(args);
    const Value* p_args[n_args] = {&args...};

    ScriptingScope scope(this);
    return mts_context_->CallClosureArgs(closure, p_args, n_args);
  }

  lepus::Value GetTopLevelVariable(bool ignore_callable = false);
  bool GetTopLevelVariableByName(const base::String& name, lepus::Value* ret);

  virtual void SetGlobalData(const base::String& name, Value value);
  /**
   * This value will overwrite the origin value
   */
  virtual void ResetGlobalData(const base::String& name, Value value);
  virtual lepus::Value GetGlobalData(const base::String& name);

  void SetGCThreshold(int64_t threshold);
  // Reads current LepusNG heap stats without triggering GC. Non-LepusNG
  // contexts return 0.
  int64_t GetCurrentHeapSizeBytes();

  const std::string& name() const { return name_; }

  void SetSourceMapRelease(const lepus::Value& source_map_release);
  void ReportErrorWithMsg(
      const std::string& msg, int32_t error_code,
      int32_t level = static_cast<int>(base::LynxErrorLevel::Error));
  void ReportErrorWithMsg(
      const std::string& msg, const std::string& stack, int32_t error_code,
      int32_t level = static_cast<int>(base::LynxErrorLevel::Error));
  void BeforeReportError(base::LynxError& error);
  void AddReporterCustomInfo(
      const std::unordered_map<std::string, std::string>& info);

  // virtual void CleanClosuresInCycleReference() {}
  void OnReload();

  void InitInspector(
      const std::shared_ptr<lepus::InspectorLepusObserver>& observer,
      const std::string& context_name);
  void PrepareInspector(const char* file_name);
  void DestroyInspector();
  std::shared_ptr<lepus::InspectorLepusObserver> UpdateInspector(
      const std::shared_ptr<lepus::InspectorLepusObserver>& observer);

  void set_name(const std::string& name) { name_ = name; }

  // check context type
  bool IsVMContext() const { return type_ == VMContextType; }
  bool IsLepusNGContext() const { return type_ == LepusNGContextType; }
  bool IsRTSContext() const { return type_ == RTSContextType; }
  bool IsRTSNativeContext() const { return type_ == RTSNativeContextType; }

  void EnsureLynx();
  void SetPropertyToLynx(const base::String& key, const lepus::Value& value);

  void ReportError(
      const std::string& exception_info,
      int32_t err_code = error::E_MTS_RUNTIME_ERROR,
      base::LynxErrorLevel error_level = base::LynxErrorLevel::Error);

  void OnBTSConsoleEvent(const std::string& func_name, const std::string& args);

  void SetSdkVersion(std::string sdk_version) {
    sdk_version_ = std::move(sdk_version);
    mts_context_->SetSdkVersion(sdk_version_);
  }

  void BindCurrentThread() { mts_context_->BindCurrentThread(); }

  // Keep old API name for MR1.
  // The actual tid binding is implemented by MTSContext (e.g. QuickContext).
  void PushContextValidTid() { BindCurrentThread(); }

  const std::string& GetSdkVersion() const { return sdk_version_; }

  const std::shared_ptr<tasm::LepusCallbackManager>& GetCallbackManager() const;
  const std::shared_ptr<tasm::AnimationFrameManager>& GetAnimationFrameManager()
      const;

  void RegisterLynx(bool enable_signal_api);

  bool DeSerialize(const ContextBundle&, bool, Value* ret,
                   const char* file_name = nullptr);

  void ApplyConfig(const std::shared_ptr<tasm::PageConfig>&,
                   const tasm::CompileOptions&);

  lepus::Value ReportFatalError(const std::string& error_message, bool exit,
                                int32_t code);

  // TODO(songshourui.null): Later, consider pushing the 'this' of LepusNG to
  // the stack, which is to avoid adding the following function on the
  // MTSRuntime class. However, pushing 'this' to the stack may lead to
  // performance degradation. If the performance test proves that pushing 'this'
  // to the stack does not cause performance degradation, then this function
  // will be deleted.
  lepus::Value GetCurrentThis(lepus::Value* argv, int32_t offset) {
    return mts_context_->GetCurrentThis(argv, offset);
  }

  void SetDebugInfoURL(const std::string& url, const std::string& file_name);

  bool IsTracingGCEnabled() { return mts_context_->IsTracingGCEnabled(); }

  void UpdateVMOuterObjSize(int size) {
    return mts_context_->UpdateVMOuterObjSize(size);
  }

  bool EvalBinary(const uint8_t* buf, uint64_t size, Value& ret,
                  const char* file_name = nullptr) {
    ScriptingScope scope(this);

    EnsureLynx();
    return mts_context_->EvalBinary(buf, size, ret, file_name);
  }

  // Execute for plain script.
  bool EvalBuf(const char* buf, uint64_t size, Value& ret,
               const char* file_name) {
    ScriptingScope scope(this);

    EnsureLynx();
    return mts_context_->EvalBuf(buf, size, ret, file_name);
  }

  void OnGC(std::string mem_info);
  void ReportGCTimingEvent(const char* start, const char* end);

 protected:
  void PushScriptingScope(ScriptingScope* scope);
  void PopScriptingScope();

  void EnsureDelegate();

  Delegate* delegate_{nullptr};

  // Inject this lynx as the global Lynx object to the Lepus runtime.
  lepus::Value lynx_;
  mutable std::shared_ptr<tasm::LepusCallbackManager> callback_manager_;
  mutable std::shared_ptr<tasm::AnimationFrameManager> animate_frame_manager_;

  ContextType type_;
  std::string name_;

  std::string sdk_version_{"null"};

  bool has_pre_execute_success_{false};

  std::unique_ptr<lepus::LepusInspectorManager> inspector_manager_;

  base::InlineStack<ScriptingScope*, 16> scripting_scope_stack_;

  JSErrorReporter js_error_reporter_;

  bool is_handling_exception_{false};
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_H_
