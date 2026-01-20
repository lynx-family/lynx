// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_module_base.h"

#include <utility>

#include "clay/fml/logging.h"

namespace lynx {

LynxModuleBase::LynxModuleBase(uint32_t view_context_id,
                               fml::RefPtr<fml::TaskRunner> task_runner)
    : view_context_id_(view_context_id), task_runner_(task_runner) {}

LynxModuleBase::~LynxModuleBase() = default;

void LynxModuleBase::RegisterInvocation(
    const lynx::runtime::NativeModuleMethod& method, Invocation&& invocation) {
  FML_DCHECK(methods_.find(method.name) == methods_.end());
  invocations_.emplace(method.name, std::move(invocation));
  methods_.emplace(method.name, std::move(method));
}

lynx::base::expected<std::unique_ptr<lynx::pub::Value>, std::string>
LynxModuleBase::InvokeMethod(const std::string& method_name,
                             std::unique_ptr<lynx::pub::Value> args,
                             size_t count,
                             const lynx::runtime::CallbackMap& callbacks) {
  if (!task_runner_) {
    return std::unique_ptr<lynx::pub::Value>(nullptr);
  }
  auto invocation_itr = invocations_.find(method_name);
  if (invocation_itr != invocations_.end() &&
      ValidateInvocation(method_name, count)) {
    return invocation_itr->second(std::move(args), callbacks);
  }
  return std::unique_ptr<lynx::pub::Value>(nullptr);
}

bool LynxModuleBase::ValidateInvocation(const std::string& method_name,
                                        size_t count) {
  auto method = methods_.find(method_name);
  if (method == methods_.end()) {
    FML_DLOG(ERROR) << "Cannot find a native method named " << method_name;
    return false;
  }
  if (method->second.args_count != count) {
    FML_DLOG(ERROR) << "Method " << method_name << "'s args count is "
                    << method->second.args_count
                    << ", which is not match input count " << count;
    return false;
  }
  return true;
}

}  // namespace lynx
