// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUSNG_QUICK_CONTEXT_H_
#define CORE_RUNTIME_LEPUSNG_QUICK_CONTEXT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/public/page_options.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/mts_context.h"
#include "core/runtime/profile/runtime_profiler.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#ifdef OS_IOS
#include "persistent-handle.h"
#else
#include "quickjs/include/persistent-handle.h"
#endif

#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

struct LEPUSRuntime;
struct LEPUSContext;

namespace lynx {
namespace lepus {

class QuickContext;
class ContextBinaryWriter;

struct QuickContextRawBindingFunction {
  const char* name;
  LEPUSCFunction* function;
  int argc = 0;
};

// QuickContextEnvWrapper always leaks, ensuring that the internal env
// pointer is always valid. This env pointer is accessed by base::Value. When
// the lifecycle of base::Value is inconsistent with QuickContext, you can check
// whether the ctx member in env is nullptr to determine if the JS Context has
// been released.
class QuickContextEnvWrapper {
 public:
  QuickContextEnvWrapper(QuickContext* qctx, LEPUSContext* ctx);
  void DetachEnv();

  lynx_api_env GetEnv() { return &env_; }

  QuickContext* GetQuickContext() const { return qctx_; }

  static inline QuickContextEnvWrapper* GetFromJsContext(LEPUSContext* ctx) {
    return ctx ? static_cast<QuickContextEnvWrapper*>(
                     LEPUS_GetContextOpaque(ctx))
               : nullptr;
  }

  static LEPUSContext* GetJsContextFromEnv(lynx_api_env env);

  static inline lynx_api_env GetEnvFromJsContext(LEPUSContext* ctx) {
    auto* env = GetFromJsContext(ctx);
    return env ? env->GetEnv() : nullptr;
  }

 private:
  QuickContext* qctx_;
  lynx_api_env__ env_;
};

class LEPUSRuntimeData {
 public:
  LEPUSRuntimeData(bool disable_tracing_gc, int runtime_mode);
  ~LEPUSRuntimeData();

  LEPUSRuntime* runtime_;
  LEPUSContext* lepus_context_;
  // "length" cache
  LEPUSAtom length_atom_;
};

// use quickjs enginer as lepus context
class QuickContext : private LEPUSRuntimeData,
                     public runtime::MTSContext,
                     public GCObserver {
 public:
  QuickContext(runtime::MTSRuntime* runtime_private,
               bool disable_tracing_gc = false, int runtime_mode = 0,
               const tasm::PageOptions& page_options = tasm::PageOptions());
  QuickContext() : QuickContext(nullptr) {}

  virtual ~QuickContext() override;
  virtual void Initialize() override;

  virtual runtime::ContextType Type() const override {
    return runtime::ContextType::LepusNGContextType;
  }

  virtual void TriggerVmGC() override;
  virtual void UpdateGCTiming(bool is_start) override;
  virtual int64_t GetCurrentHeapSizeBytes() override;

  virtual bool UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                            const lepus::Value& val) override;
  virtual bool CheckTableShadowUpdatedWithTopLevelVariable(
      const lepus::Value& update) override;
  virtual void ResetTopLevelVariable() override;
  virtual void ResetTopLevelVariableByVal(const Value& val) override;

  virtual lepus::Value GetTopLevelVariable(
      bool ignore_callable = false) override;

  LEPUSContext* context() const { return lepus_context_; }

  bool GetTopLevelVariableByName(const base::String& name,
                                 lepus::Value* ret) override;

  virtual void SetGlobalData(const base::String& name, Value value) override;
  virtual void ResetGlobalData(const base::String& name, Value value) override;
  virtual lepus::Value GetGlobalData(const base::String& name) override;

  virtual void SetGCThreshold(int64_t threshold) override;

  void SetEnableStrictCheck(bool val);
  void SetStackSize(uint32_t stack_size);
  void RegisterGlobalFunction(const char* name, LEPUSCFunction* func,
                              int argc = 0);
  void RegisterGlobalFunction(const runtime::RenderBindingFunction* funcs,
                              size_t size) override;
  void RegisterObjectFunction(lepus::Value& obj,
                              const runtime::RenderBindingFunction* funcs,
                              size_t size) override;
  void RegisterGlobalFunction(const QuickContextRawBindingFunction* funcs,
                              size_t size);

  virtual Value CallArgs(const base::String& name, const Value* args[],
                         size_t args_count,
                         bool pause_suppression_mode) override;
  virtual Value CallClosureArgs(const Value& closure, const Value* args[],
                                size_t args_count) override;

  LEPUSValue NewBindingFunction(CFunction func);

  /**
   * @brief Called when the garbage collection (GC) operation is completed to
   * handle the post-GC memory information.
   *
   * This method is a concrete implementation of the `GCObserver` interface. It
   * will receive a string containing memory-related information after the
   * garbage collection operation ends.
   *
   * @param mem_info A string containing post-GC memory information. Its format
   * could be JSON or a specific custom format.
   */
  void OnGC(std::string mem_info) override;

  void RegisterGlobalProperty(const char* name, LEPUSValue val);

  LEPUSValue SearchGlobalData(const std::string& name);

  // deserialize
  bool DeSerialize(const runtime::ContextBundle&, bool, Value* ret,
                   const char* file_name = nullptr) override;

  // DeSerialize & Execute
  bool EvalBinary(const uint8_t* buf, uint64_t size, Value& ret,
                  const char* file_name = nullptr) override;

  // Execute for plain script.
  bool EvalBuf(const char* buf, uint64_t size, Value& ret,
               const char* file_name) override;

  LEPUSValue GetAndCall(const std::string& name, LEPUSValue*, size_t);
  LEPUSValue InternalCall(LEPUSValue func, LEPUSValue*, size_t);
  void SetProperty(const char* name, LEPUSValue obj, LEPUSValue val);

  inline void set_napi_env(void* env) { napi_env_ = env; }
  inline void* napi_env() { return napi_env_; }

  void SetDebuggerSourceAndEndLine(const std::string& source);

  LEPUSValue ReportSetConstValueError(const LEPUSValue&, LEPUSValue);

  void set_debuginfo_outside(bool val);
  bool debuginfo_outside() const override;

  void ApplyConfig(const std::shared_ptr<tasm::PageConfig>&,
                   const tasm::CompileOptions&) override;

  LEPUSAtom GetLengthAtom() const { return length_atom_; }

  void SetFunctionFileName(LEPUSValue func_obj, const char* file_name);

  lepus::Value ReportFatalError(const std::string& error_message, bool exit,
                                int32_t code) override;

  std::string FormatExceptionMessage(const std::string& message,
                                     const std::string& stack,
                                     const std::string& prefix) override;

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

  // TODO(wangboyong): refact this
  bool Execute();
  bool ExecuteBinaryWithBundle(const runtime::ContextBundle* bundle,
                               Value* ret_val) override;

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
  void PrepareInspector(const char* file_name) override;

#if ENABLE_TRACE_PERFETTO
  void SetRuntimeProfiler(
      std::shared_ptr<runtime::profile::RuntimeProfiler> runtime_profile);
  void RemoveRuntimeProfiler();
#endif

  virtual void UpdateVMOuterObjSize(int size) override;

  virtual bool IsTracingGCEnabled() override;

  virtual void BindCurrentThread() override;

  void SetDebugSourceCode(const std::string& source) { debug_source_ = source; }
  const std::string& GetDebugSourceCode() const { return debug_source_; }

  static QuickContext* GetFromJsContext(LEPUSContext* ctx) {
    auto* env = QuickContextEnvWrapper::GetFromJsContext(ctx);
    return env ? env->GetQuickContext() : nullptr;
  }

 private:
  static LEPUSLepusRefCallbacks GetLepusRefCall();

  bool ExecuteBinaryInternal(Value* ret);

  LEPUSValue GetProperty(const std::string& name, LEPUSValue this_obj);

  void RegisterLepusVerion();
  void EnableRuntimeLeakCheck(bool enable);

  void EvalLepusPendingTask();
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

  // debugger source code
  std::string debug_source_;

#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<runtime::profile::RuntimeProfiler> runtime_profiler_;
#endif
};

class QuickContextBundle final : public runtime::ContextBundle {
 public:
  QuickContextBundle() = default;
  virtual ~QuickContextBundle() override = default;
  virtual bool IsLepusNG() const override;
  virtual bool IsRTS() const override;
  virtual bool IsRTSNative() const override;

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
#endif  // CORE_RUNTIME_LEPUSNG_QUICK_CONTEXT_H_
