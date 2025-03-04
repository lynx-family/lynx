// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/lynx_module.h"

#include <memory>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/include/expected.h"
#include "base/include/no_destructor.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
namespace LynxModuleUtils {
std::string JSTypeToString(const piper::Value* arg) {
  if (!arg) {
    return "nullptr";
  }
  return arg->typeToString();
}

std::string ExpectedButGot(const std::string& expected,
                           const std::string& but_got) {
  return std::string{"expected: "}
      .append(expected)
      .append(", but got ")
      .append(but_got)
      .append(".");
}

std::string ExpectedButGotAtIndexError(const std::string& expected,
                                       const std::string& but_got,
                                       int arg_index) {
  auto message = std::string{" argument: "};
  message += std::to_string(arg_index);
  message += ", expected: ";
  message += expected;
  message += ", but got ";
  message += but_got;
  message += ".";
  return message;
}

std::string ExpectedButGotError(int expected, int but_got) {
  return " invoked with wrong number of arguments," +
         ExpectedButGot(std::to_string(expected), std::to_string(but_got));
}

std::string GenerateErrorMessage(const std::string& module,
                                 const std::string& method,
                                 const std::string& error) {
  auto message = std::string{"In module '"}.append(module);
  message.append("' method '").append(method);
  message.append("' :").append(error);
  return message;
}

}  // namespace LynxModuleUtils

// AllowList For Special Methods
// see issue: #1979
const std::unordered_set<std::string>& LynxModule::MethodAllowList() {
  static const base::NoDestructor<std::unordered_set<std::string>>
      method_allow_list({"splice", "then"});
  return *method_allow_list;
}

LynxModule::MethodMetadata::MethodMetadata(size_t count,
                                           const std::string& methodName)
    : argCount(count), name(methodName) {}

piper::Value LynxModule::get(Runtime* runtime, const PropNameID& prop) {
  std::string propNameUtf8 = prop.utf8(*runtime);
  auto p = methodMap_.find(propNameUtf8);

  if (p != methodMap_.end()) {
    auto& meta = p->second;
    return piper::Function::createFromHostFunction(
        *runtime, prop, static_cast<unsigned int>(meta->argCount),
        [meta, propNameUtf8,
         weak_self = std::weak_ptr<LynxModule>(shared_from_this())](
            Runtime& rt, const Value& thisVal, const Value* args,
            size_t count) -> base::expected<Value, JSINativeException> {
          auto lock_module = weak_self.lock();
          if (!lock_module) {
            LOGE("LynxModule has been destroyed.");
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("LynxModule has been destroyed."));
          }
          if (meta.get() == nullptr) {
            LOGE("LynxModule, module: "
                 << lock_module->name_
                 << " failed in invokeMethod(), method is a nullptr");
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "LynxModule, module:" + lock_module->name_ +
                " failed in invokeMethod(), method is nullptr"));
          }
          return lock_module->invokeMethod(*(meta.get()), &rt, args, count);
        });
  } else {
    // AllowList For Special Methods
    // see issue: #1979
    if (!MethodAllowList().count(propNameUtf8)) {
      LOGI("module: " << name_ << ", method: " << propNameUtf8
                      << " cannot be found in the method map");
      delegate_->OnMethodInvoked(
          name_, propNameUtf8,
          error::E_NATIVE_MODULES_COMMON_FUNCTION_NOT_FOUND);
    }
    return piper::Value::undefined();
  }

  // TODO: All these code related to LynxAttribute are dead code, can be
  // removed.
  return this->getAttributeValue(runtime, propNameUtf8);
}
}  // namespace piper
}  // namespace lynx
