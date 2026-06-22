// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_MTS_CONTEXT_H_
#define CORE_RUNTIME_MTS_CONTEXT_H_

#include <cstdint>
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
#include "core/public/page_options.h"
#include "core/runtime/lepus/lepus_global.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace runtime {

class MTSRuntime;

using lepus::CFunction;
using lepus::Value;

enum ContextType {
  VMContextType,         // Run low level version lepus with VmContext
  LepusNGContextType,    // Run lepusNG with qucikjs code
  RTSContextType,        // Run RTS with VmContext
  RTSNativeContextType,  // Run RTS with NativeContext
};

struct RenderBindingFunction {
  const char* name;
  CFunction function;
  bool for_lepus = true;
  bool for_lepusng = true;
};

class ContextBundle {
 public:
  ContextBundle() = default;
  virtual ~ContextBundle() = default;
  virtual bool IsLepusNG() const = 0;
  virtual bool IsRTS() const = 0;
  virtual bool IsRTSNative() const = 0;
  virtual void OnCustomSectionDecoded(const std::string& key,
                                      const Value& value) {}

  static std::unique_ptr<ContextBundle> Create(ContextType context_type);
};

class MTSContext {
 public:
  virtual ~MTSContext() {}
  explicit MTSContext(MTSRuntime* runtime_private)
      : runtime_private_(runtime_private) {}

  // virtual interface
  virtual void Initialize() = 0;

  virtual ContextType Type() const = 0;
  virtual std::string GetDebugDescription() const = 0;

  bool IsVMContext() const { return Type() == ContextType::VMContextType; }
  bool IsLepusNGContext() const {
    return Type() == ContextType::LepusNGContextType;
  }

  base::StringTable* string_table() { return &string_table_; }
  const base::StringTable* string_table() const { return &string_table_; }

  void set_is_debug_enabled(bool is_debug_enabled) {
    is_debug_enabled_ = is_debug_enabled;
  }
  virtual void PrepareInspector(const char* file_name) {}

  virtual void UpdateGCTiming(bool is_start) {}
  virtual void TriggerVmGC() {}
  virtual int64_t GetCurrentHeapSizeBytes() { return 0; }

  virtual bool EnableSendEventToMainThread() const { return false; }

  virtual void RegisterGlobalFunction(const RenderBindingFunction* funcs,
                                      size_t size) = 0;
  virtual void RegisterObjectFunction(lepus::Value& obj,
                                      const RenderBindingFunction* funcs,
                                      size_t size) = 0;

  /**
   * TODO, need this ? use only radonPage
   */
  virtual bool UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                            const Value& val) = 0;
  virtual bool CheckTableShadowUpdatedWithTopLevelVariable(
      const lepus::Value& update) = 0;

  // only for lepus
  virtual void ResetTopLevelVariable(){};
  virtual void ResetTopLevelVariableByVal(const Value& val){};
  virtual lepus::Value GetTopLevelVariable(bool ignore_callable = false) {
    return lepus::Value();
  }
  virtual bool GetTopLevelVariableByName(const base::String& name,
                                         lepus::Value* ret) {
    return true;
  };

  virtual void SetGlobalData(const base::String& name, Value value) = 0;
  /**
   * This value will overwrite the origin value
   */
  virtual void ResetGlobalData(const base::String& name, Value value) = 0;
  virtual lepus::Value GetGlobalData(const base::String& name) = 0;

  // only for lepusNG
  virtual void SetGCThreshold(int64_t threshold){};

  virtual void OnReload() {}

  // 暂时保留，但是对于rts 不提供类似接口
  virtual bool DeSerialize(const ContextBundle&, bool, Value* ret,
                           const char* file_name = nullptr) = 0;

  virtual void ApplyConfig(const std::shared_ptr<tasm::PageConfig>&,
                           const tasm::CompileOptions&) = 0;

  virtual lepus::Value ReportFatalError(const std::string& error_message,
                                        bool exit, int32_t code) = 0;

  virtual std::string FormatExceptionMessage(const std::string& message,
                                             const std::string& stack,
                                             const std::string& prefix) {
    return prefix + message + "\n" + stack;
  }

  // TODO(songshourui.null): Later, consider pushing the 'this' of LepusNG to
  // the stack, which is to avoid adding the following function on the Context
  // class. However, pushing 'this' to the stack may lead to performance
  // degradation. If the performance test proves that pushing 'this' to the
  // stack does not cause performance degradation, then this function will be
  // deleted.
  virtual lepus::Value GetCurrentThis(lepus::Value* argv, int32_t offset) {
    return lepus::Value();
  }

  // used for lepus、lepusNg
  virtual void SetDebugInfoURL(const std::string& url,
                               const std::string& file_name){};

  // for memory gc
  virtual void UpdateVMOuterObjSize(int size){};
  virtual bool IsTracingGCEnabled() { return false; }

  virtual void BindCurrentThread() = 0;

  virtual bool EvalBinary(const uint8_t* buf, uint64_t size, Value& ret,
                          const char* file_name = nullptr) {
    return false;
  }

  // Execute for plain script.
  virtual bool EvalBuf(const char* buf, uint64_t size, Value& ret,
                       const char* file_name) {
    return false;
  }

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
  virtual Value CallArgs(const base::String& name, const Value* args[],
                         size_t args_count, bool pause_suppression_mode) = 0;
  virtual Value CallClosureArgs(const Value& closure, const Value* args[],
                                size_t args_count) = 0;

  // Execute with optional ContextBundle (e.g. RTSNative uses bundle_id).
  // NOTE: This is the unified execute entry; pass `nullptr` when bundle is not
  // available.
  virtual bool ExecuteBinaryWithBundle(const ContextBundle* bundle = nullptr,
                                       Value* ret_val = nullptr) = 0;
  virtual bool debuginfo_outside() const { return true; }

  void SetSdkVersion(std::string sdk_version) {
    sdk_version_ = std::move(sdk_version);
  }
  const std::string& GetSdkVersion() const { return sdk_version_; }

  void* GetRuntimePrivate() const {
    return reinterpret_cast<void*>(runtime_private_);
  }

  void OnBTSConsoleEvent(const std::string& func_name, const std::string& args);

  void ReportErrorWithMsg(
      const std::string& msg, int32_t error_code,
      int32_t level = static_cast<int>(base::LynxErrorLevel::Error));

  void ReportError(
      const std::string& exception_info,
      int32_t err_code = error::E_MTS_RUNTIME_ERROR,
      base::LynxErrorLevel error_level = base::LynxErrorLevel::Error);

  void ReportGCTimingEvent(const char* start, const char* end);

  void OnContextGC(std::unordered_map<std::string, std::string> mem_info);

 protected:
  // Inject this lynx as the global Lynx object to the Lepus runtime.
  lepus::Value lynx_;
  base::StringTable string_table_;
  std::string sdk_version_{"null"};
  bool is_debug_enabled_{false};
  MTSRuntime* runtime_private_{nullptr};
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_MTS_CONTEXT_H_
