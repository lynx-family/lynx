// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_QUICK_CONTEXT_H_
#define CORE_RUNTIME_VM_LEPUS_QUICK_CONTEXT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/profile/runtime_profiler.h"
#include "core/runtime/vm/lepus/context.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

namespace lynx {
namespace lepus {

class ContextBinaryWriter;
using RenderBindingFunc = lepus::Value (*)(lepus::Context*, lepus::Value*,
                                           int32_t);

struct RenderBindingFunction {
  const char* name;
  RenderBindingFunc function;
};

// use quickjs enginer as lepus context
class QuickContext : private LEPUSRuntimeData, public Context {
 public:
  static QuickContext* Cast(Context* context);
  QuickContext(bool disable_tracing_gc = false);
  virtual ~QuickContext();
  virtual void Initialize() override;
  virtual bool Execute(Value* ret = nullptr) override;

  virtual void UpdateGCTiming(bool is_start) override;

  virtual long GetParamsSize() override;
  virtual Value* GetParam(long index) override;
  virtual const std::string& name() const override;
  virtual bool UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                            const lepus::Value& val) override;
  virtual bool CheckTableShadowUpdatedWithTopLevelVariable(
      const lepus::Value& update) override;
  virtual void ResetTopLevelVariable() override;
  virtual void ResetTopLevelVariableByVal(const Value& val) override;

  virtual std::unique_ptr<lepus::Value> GetTopLevelVariable(
      bool ignore_callable = false) override;
  LEPUSContext* context() const override { return lepus_context_; }
  bool GetTopLevelVariableByName(const base::String& name,
                                 lepus::Value* ret) override;

  virtual void SetGlobalData(const base::String& name, Value value) override;
  virtual lepus::Value GetGlobalData(const base::String& name) override;

  virtual void SetGCThreshold(int64_t threshold) override;

  void SetEnableStrictCheck(bool val);
  void SetStackSize(uint32_t stack_size);
  void RegisterGlobalFunction(const char* name, LEPUSCFunction* func,
                              int argc = 0);
  void RegisterGlobalFunction(const RenderBindingFunction* funcs, size_t size);
  void RegisterObjectFunction(lepus::Value& obj,
                              const RenderBindingFunction* funcs, size_t size);

  LEPUSValue NewBindingFunction(RenderBindingFunc func);

  void RegisterGlobalProperty(const char* name, LEPUSValue val);

  LEPUSValue SearchGlobalData(const std::string& name);

  // deserialize
  bool DeSerialize(const ContextBundle&, bool, Value* ret,
                   const char* file_name = nullptr) override;

  // DeSerialize & Execute
  bool EvalBinary(const uint8_t* buf, uint64_t size, Value* ret,
                  const char* file_name = nullptr);

  LEPUSValue GetAndCall(const std::string& name, LEPUSValue*, size_t);
  LEPUSValue InternalCall(LEPUSValue func, LEPUSValue*, size_t);
  void SetProperty(const char* name, LEPUSValue obj, LEPUSValue val);

  inline void set_napi_env(void* env) { napi_env_ = env; }
  inline void* napi_env() { return napi_env_; }

  virtual void RegisterMethodToLynx() override;

  virtual void RegisterLepusVerion() override;
  void SetDebuggerSourceAndEndLine(const std::string& source);
  BASE_EXPORT_FOR_DEVTOOL LEPUSValue GetTopLevelFunction() const override;

  LEPUSValue ReportSetConstValueError(const LEPUSValue&, LEPUSValue);

  void set_debuginfo_outside(bool val);
  bool debuginfo_outside() const;

  void SetSourceMapRelease(const lepus::Value& source_map_release) override;
  void ReportErrorWithMsg(
      const std::string& msg, int32_t error_code,
      int32_t level = static_cast<int>(base::LynxErrorLevel::Error)) override;
  void ReportErrorWithMsg(
      const std::string& msg, const std::string& stack, int32_t error_code,
      int32_t level = static_cast<int>(base::LynxErrorLevel::Error)) override;
  void AddReporterCustomInfo(
      const std::unordered_map<std::string, std::string>& info) override;
  void BeforeReportError(base::LynxError& error) override;

  void RegisterCtxBuiltin(const tasm::ArchOption&) override;
  void ApplyConfig(const std::shared_ptr<tasm::PageConfig>&,
                   const tasm::CompileOptions&) override;

  LEPUSAtom GetLengthAtom() const { return length_atom_; }

  void SetFunctionFileName(LEPUSValue func_obj, const char* file_name);

  static QuickContext* GetFromJsContext(LEPUSContext* ctx) {
    auto* cell = GetContextCellFromCtx(ctx);
    return cell ? cell->qctx_ : nullptr;
  }

  lepus::Value ReportFatalError(const std::string& error_message, bool exit,
                                int32_t code) override;

  virtual lepus::Value GetCurrentThis(lepus::Value* argv,
                                      int32_t offset) override;

  void set_current_this(LEPUSValue current_this) {
    current_this_ = current_this;
  }

  bool GetDebuginfoOutside() { return debuginfo_outside_; }

  void SetTopLevelFunction(LEPUSValue val);

  std::string GetExceptionMessage(const char* prefix = "",
                                  int32_t* err_code = nullptr);

  LEPUSValue& GetTopLevelFunction() { return top_level_function_; }

  bool GetGCFlag() { return gc_flag_; }

  class DebugDelegate {
   public:
    virtual ~DebugDelegate() = default;
    virtual void OnTopLevelFunctionReady() = 0;
  };
  void SetDebugDelegate(const std::shared_ptr<DebugDelegate>& debug_delegate) {
    debug_delegate_ = debug_delegate;
  }
  const std::weak_ptr<DebugDelegate> GetDebugDelegate() {
    return debug_delegate_;
  }

#if ENABLE_TRACE_PERFETTO
  void SetRuntimeProfiler(
      std::shared_ptr<profile::RuntimeProfiler> runtime_profile);
  void RemoveRuntimeProfiler();
#endif

 private:
  virtual Value CallArgs(const base::String& name, const Value* args[],
                         size_t args_count,
                         bool pause_suppression_mode) override;
  virtual Value CallClosureArgs(const Value& closure, const Value* args[],
                                size_t args_count) override;

  std::string FormatExceptionMessage(const std::string& message,
                                     const std::string& stack,
                                     const std::string& prefix);

  LEPUSValue GetProperty(const std::string& name, LEPUSValue this_obj);
  LEPUSValue top_level_function_;
  GCPersistent p_val_;

  // TODO: optimize it
  // runtime_ may can shared between context
  bool use_lepus_strict_mode_;
  uint32_t stack_size_ = 0;

  // Napi::Env
  void* napi_env_{nullptr};

  // for debug
  std::weak_ptr<DebugDelegate> debug_delegate_;
  bool debuginfo_outside_;
  bool gc_flag_;

  LEPUSValue current_this_;
  char* gc_info_start_;

  common::JSErrorReporter js_error_reporter_;
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<profile::RuntimeProfiler> runtime_profiler_;
#endif
};

class QuickContextBundle final : public ContextBundle {
 public:
  QuickContextBundle() = default;
  virtual ~QuickContextBundle() override = default;
  virtual bool IsLepusNG() const override;

  std::vector<uint8_t>& lepus_code() { return lepusng_code_; }
  uint64_t& lepusng_code_len() { return lepusng_code_len_; }

 private:
  std::vector<uint8_t> lepusng_code_{};
  uint64_t lepusng_code_len_{0};
  friend class QuickContextDecoder;
  friend class QuickContext;
};

}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_VM_LEPUS_QUICK_CONTEXT_H_
