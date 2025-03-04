// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_CONTEXT_H_
#define CORE_RUNTIME_VM_LEPUS_CONTEXT_H_

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "base/include/value/base_string.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/inspector/lepus_inspector_manager.h"
#include "core/inspector/observer/inspector_lepus_observer.h"
#include "core/renderer/page_config.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/vm/lepus/lepus_global.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/path_parser.h"
#include "core/template_bundle/template_codec/compile_options.h"

struct LEPUSRuntime;
struct LEPUSContext;

namespace lynx {

namespace tasm {
class AnimationFrameManager;
class LepusCallbackManager;
}  // namespace tasm

namespace lepus {
class ContextBundle;

class LEPUSRuntimeData {
 public:
  LEPUSRuntimeData(bool disable_tracing_gc);
  ~LEPUSRuntimeData();

  LEPUSRuntime* runtime_;
  LEPUSContext* lepus_context_;
  // "length" cache
  LEPUSAtom length_atom_;
};

enum ContextType {
  VMContextType,       // Run low level version lepus with VmContext
  LepusNGContextType,  // Run lepusNG with qucikjs code
  LepusContextType     // Run low level version lepus with LepusNG
};

#define LEPUS_DEFAULT_CONTEXT_NAME "__Card__"

class Context {
 public:
  class Delegate {
   public:
    virtual const std::string& TargetSdkVersion() = 0;
    virtual void ReportError(base::LynxError error) = 0;
    virtual void PrintMsgToJS(const std::string& level,
                              const std::string& msg) = 0;
    virtual void ReportGCTimingEvent(const char* start, const char* end) = 0;

    virtual fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() = 0;
  };

  virtual ~Context() {}
  Context(ContextType type);

  Delegate* GetDelegate();

  // virtual interface
  virtual void Initialize() = 0;

  virtual bool Execute(Value* ret = nullptr) = 0;

  virtual void UpdateGCTiming(bool is_start){};

  bool UpdateTopLevelVariable(const std::string& name, const Value& val);
  virtual bool UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                            const Value& val) = 0;
  // shadow equal for table
  virtual bool CheckTableShadowUpdatedWithTopLevelVariable(
      const lepus::Value& update) = 0;

  virtual void ResetTopLevelVariable() = 0;
  virtual void ResetTopLevelVariableByVal(const Value& val) = 0;

  Value CallArgs(const base::String& name, const std::vector<Value>& args,
                 bool pause_suppression_mode = false);
  Value CallClosureArgs(const Value& closure, const std::vector<Value>& args);

  template <class... Args,
            class = std::enable_if_t<
                (std::is_same_v<
                     Value, std::remove_cv_t<std::remove_reference_t<Args>>> &&
                 ...)>>
  Value Call(const base::String& name, const Args&... args) {
    constexpr auto n_args = sizeof...(args);
    const Value* p_args[n_args] = {&args...};
    return CallArgs(name, p_args, n_args, false);
  }

  template <class... Args,
            class = std::enable_if_t<
                (std::is_same_v<
                     Value, std::remove_cv_t<std::remove_reference_t<Args>>> &&
                 ...)>>
  Value CallInPauseSuppressionMode(const base::String& name, Args&&... args) {
    constexpr auto n_args = sizeof...(args);
    const Value* p_args[n_args] = {&args...};
    return CallArgs(name, p_args, n_args, true);
  }

  template <class... Args,
            class = std::enable_if_t<
                (std::is_same_v<
                     Value, std::remove_cv_t<std::remove_reference_t<Args>>> &&
                 ...)>>
  Value CallClosure(const Value& closure, Args&&... args) {
    constexpr auto n_args = sizeof...(args);
    const Value* p_args[n_args] = {&args...};
    return CallClosureArgs(closure, p_args, n_args);
  }

  virtual std::unique_ptr<Value> GetTopLevelVariable(
      bool ignore_callable = false) = 0;
  virtual bool GetTopLevelVariableByName(const base::String& name,
                                         lepus::Value* ret) = 0;

  virtual long GetParamsSize() = 0;
  virtual Value* GetParam(long index) = 0;
  virtual void SetGlobalData(const base::String& name, Value value) = 0;
  virtual lepus::Value GetGlobalData(const base::String& name) = 0;

  virtual void SetGCThreshold(int64_t threshold){};

  virtual const std::string& name() const { return name_; }

  virtual void SetSourceMapRelease(const lepus::Value& source_map_release){};
  virtual void ReportErrorWithMsg(
      const std::string& msg, int32_t error_code,
      int32_t level = static_cast<int>(base::LynxErrorLevel::Error)){};
  virtual void ReportErrorWithMsg(
      const std::string& msg, const std::string& stack, int32_t error_code,
      int32_t level = static_cast<int>(base::LynxErrorLevel::Error)){};
  virtual void BeforeReportError(base::LynxError& error) {}
  virtual void AddReporterCustomInfo(
      const std::unordered_map<std::string, std::string>& info){};

  virtual void CleanClosuresInCycleReference() {}

  void InitInspector(const std::shared_ptr<InspectorLepusObserver>& observer);
  void DestroyInspector();

  base::StringTable* string_table() { return &string_table_; }
  void set_name(const std::string& name) { name_ = name; }

  static std::shared_ptr<Context> CreateContext(
      bool use_lepusng = false, bool disable_tracing_gc = false);

  // check context type
  bool IsVMContext() const { return type_ == VMContextType; }
  bool IsLepusNGContext() const { return type_ == LepusNGContextType; }
  bool IsLepusContext() const { return type_ == LepusContextType; }
  virtual LEPUSContext* context() const { return nullptr; }
  virtual LEPUSValue GetTopLevelFunction() const { return LEPUS_UNDEFINED; }

  static LEPUSLepusRefCallbacks GetLepusRefCall();

  static CellManager& GetContextCells();
  static ContextCell* RegisterContextCell(lepus::QuickContext* qctx);

  static inline ContextCell* GetContextCellFromCtx(LEPUSContext* ctx) {
    return ctx ? reinterpret_cast<ContextCell*>(LEPUS_GetContextOpaque(ctx))
               : nullptr;
  }

  void EnsureLynx();
  void SetPropertyToLynx(const base::String& key, const lepus::Value& value);
  virtual void RegisterMethodToLynx() {}

  void ReportError(
      const std::string& exception_info,
      int32_t err_code = error::E_MTS_RUNTIME_ERROR,
      base::LynxErrorLevel error_level = base::LynxErrorLevel::Error);

  void PrintMsgToJS(const std::string& level, const std::string& msg);

  virtual void RegisterLepusVerion() = 0;

  void SetSdkVersion(std::string sdk_version) {
    sdk_version_ = std::move(sdk_version);
  }

  const std::string& GetSdkVersion() const { return sdk_version_; }

  void SetDebugSourceCode(const std::string& source) { debug_source_ = source; }
  const std::string& GetDebugSourceCode() const { return debug_source_; }

  const std::shared_ptr<tasm::LepusCallbackManager>& GetCallbackManager() const;
  const std::shared_ptr<tasm::AnimationFrameManager>& GetAnimationFrameManager()
      const;

  virtual bool DeSerialize(const ContextBundle&, bool, Value* ret,
                           const char* file_name = nullptr) = 0;

  virtual void RegisterCtxBuiltin(const tasm::ArchOption&) = 0;
  virtual void ApplyConfig(const std::shared_ptr<tasm::PageConfig>&,
                           const tasm::CompileOptions&) = 0;

  virtual lepus::Value ReportFatalError(const std::string& error_message,
                                        bool exit, int32_t code) = 0;

  // TODO(songshourui.null): Later, consider pushing the 'this' of LepusNG to
  // the stack, which is to avoid adding the following function on the Context
  // class. However, pushing 'this' to the stack may lead to performance
  // degradation. If the performance test proves that pushing 'this' to the
  // stack does not cause performance degradation, then this function will be
  // deleted.
  virtual lepus::Value GetCurrentThis(lepus::Value* argv, int32_t offset) {
    return lepus::Value();
  }

  void SetDebugInfoURL(const std::string& url) { debug_info_url_ = url; }

  const std::string& GetDebugInfoURL() { return debug_info_url_; }

 protected:
  virtual Value CallArgs(const base::String& name, const Value* args[],
                         size_t args_count, bool pause_suppression_mode) = 0;
  virtual Value CallClosureArgs(const Value& closure, const Value* args[],
                                size_t args_count) = 0;

  void EnsureDelegate();

  Delegate* delegate_{nullptr};

  // Inject this lynx as the global Lynx object to the Lepus runtime.
  lepus::Value lynx_;
  mutable std::shared_ptr<tasm::LepusCallbackManager> callback_manager_;
  mutable std::shared_ptr<tasm::AnimationFrameManager> animate_frame_manager_;

  ContextType type_;
  std::string name_;
  base::StringTable string_table_;

  std::string sdk_version_{"null"};
  // debugger source code
  std::string debug_source_;

  std::string debug_info_url_;
  std::unique_ptr<LepusInspectorManager> inspector_manager_;
};

class ContextBundle {
 public:
  ContextBundle() = default;
  virtual ~ContextBundle() = default;
  virtual bool IsLepusNG() const = 0;

  static std::unique_ptr<ContextBundle> Create(bool is_lepusng_binary);
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_CONTEXT_H_
