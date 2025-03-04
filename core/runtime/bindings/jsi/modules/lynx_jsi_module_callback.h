// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_JSI_MODULE_CALLBACK_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_JSI_MODULE_CALLBACK_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "core/public/jsb/lynx_module_callback.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_timing.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace runtime {
class TemplateDelegate;
}
}  // namespace lynx

namespace lynx {
namespace piper {

class ModuleCallbackFunctionHolder {
 public:
  ModuleCallbackFunctionHolder(piper::Function&& func);
  ~ModuleCallbackFunctionHolder() = default;
  piper::Function function_;
};

// TODO(huzhanbo.luc): move this into request_interceptor
enum class ModuleCallbackType {
  Base,
  Request,
  Fetch,
};

class ModuleCallback : public LynxModuleCallback {
 public:
  static constexpr int64_t kInvalidCallbackId = -1;
  explicit ModuleCallback(int64_t callback_id);
  virtual ~ModuleCallback() = default;
  virtual void Invoke(Runtime* runtime, ModuleCallbackFunctionHolder* holder);
  void ReportLynxErrors(runtime::TemplateDelegate* delegate);
  int64_t callback_id() const { return callback_id_; }
  void SetModuleName(const std::string& module_name) {
    module_name_ = module_name;
  }
  void SetMethodName(const std::string& method_name) {
    method_name_ = method_name;
  }
  void SetFirstArg(const std::string& first_arg) { first_arg_ = first_arg; }
  std::string module_name_;
  std::string method_name_;
  // Some JSB implement will use first arg as JSB function name,
  // so we need first arg for tracing.
  std::string first_arg_;
  uint64_t start_time_ms_ = 0;
  const std::string& FirstArg() const { return first_arg_; }
  void SetStartTimeMS(uint64_t ms) { start_time_ms_ = ms; }
  uint64_t StartTimeMS() const { return start_time_ms_; }

  NativeModuleInfoCollectorPtr timing_collector_;

  void SetArgs(std::unique_ptr<pub::Value> args) override;

#if ENABLE_TESTBENCH_RECORDER
  void SetRecordID(int64_t record_id);
#endif
  int64_t record_id_ = 0;

 protected:
  std::vector<base::LynxError> errors_;

 private:
  std::unique_ptr<pub::Value> args_ = nullptr;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_JSI_MODULE_CALLBACK_H_
