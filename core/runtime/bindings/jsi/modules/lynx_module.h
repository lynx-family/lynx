// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/expected.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
namespace LynxModuleUtils {
// module error message creating
std::string JSTypeToString(const piper::Value* arg);
std::string ExpectedButGotAtIndexError(const std::string& expected,
                                       const std::string& but_got,
                                       int arg_index);
std::string ExpectedButGotError(int expected, int but_got);

std::string GenerateErrorMessage(const std::string& module,
                                 const std::string& method,
                                 const std::string& error);
}  // namespace LynxModuleUtils

class GroupInterceptor;

/**
 * Base HostObject class for every module to be exposed to JS
 */
class LynxModule : public piper::HostObject,
                   public std::enable_shared_from_this<LynxModule> {
 public:
  LynxModule(const std::string& name,
             const std::shared_ptr<ModuleDelegate>& delegate)
      : name_(name), delegate_(delegate) {}
  ~LynxModule() override = default;
  virtual void Destroy() = 0;

  piper::Value get(Runtime* rt, const PropNameID& prop) override;

  const std::string name_;
  // Public for NetworkInterceptor. When Refactor network finished, protected
  // this.
  const std::shared_ptr<ModuleDelegate> delegate_;

  class MethodMetadata {
   public:
    const size_t argCount;
    const std::string name;
    MethodMetadata(size_t argCount, const std::string& methodName);
  };

  virtual base::expected<piper::Value, piper::JSINativeException> invokeMethod(
      const MethodMetadata& method, Runtime* rt, const piper::Value* args,
      size_t count) = 0;

  // TODO(chenyouhui): It is dead code. Add a default implementation and remove
  // it later.
  virtual piper::Value getAttributeValue(Runtime* rt, std::string propName) {
    return piper::Value::undefined();
  }

  void SetModuleInterceptor(std::shared_ptr<GroupInterceptor> interceptor) {
    group_interceptor_ = std::move(interceptor);
  }

#if ENABLE_TESTBENCH_RECORDER
  /*
   *SetRecordID, GetRecordID, EndRecordFunction and StartRecordFunction only
   *used by TestBench
   */
  virtual void SetRecordID(int64_t record_id) { record_id_ = record_id; }
  int64_t GetRecordID() { return record_id_; }
  virtual void EndRecordFunction(const std::string& method_name, size_t count,
                                 const piper::Value* js_args, Runtime* rt,
                                 piper::Value& res) {}
  virtual void StartRecordFunction(const std::string& method_name = "") {}
#endif  // ENABLE_TESTBENCH_RECORDER

 protected:
  std::unordered_map<std::string, std::shared_ptr<MethodMetadata>> methodMap_;
  std::shared_ptr<GroupInterceptor> group_interceptor_;
#if ENABLE_TESTBENCH_RECORDER
  ALLOW_UNUSED_TYPE int64_t record_id_ = 0;
#endif  // ENABLE_TESTBENCH_RECORDER

 private:
  static const std::unordered_set<std::string>& MethodAllowList();
};

/**
 * An app/platform-specific provider function to get an instance of a module
 * given a name.
 */
using LynxModuleProviderFunction =
    std::function<std::shared_ptr<LynxModule>(const std::string& name)>;

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_H_
