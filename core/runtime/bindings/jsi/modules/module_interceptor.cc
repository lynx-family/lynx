// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/bindings/jsi/modules/module_interceptor.h"

#include <utility>

namespace lynx {
namespace piper {

ModuleInterceptorResult GroupInterceptor::InterceptModuleMethod(
    const std::shared_ptr<LynxModule>& module,
    const LynxModule::MethodMetadata& method, Runtime* rt,
    const std::shared_ptr<piper::ModuleDelegate>& delegate,
    const piper::Value* args, size_t count) const {
  for (auto& i : interceptors_) {
    auto pair =
        i->InterceptModuleMethod(module, method, rt, delegate, args, count);
    if (pair.handled) {
      return pair;
    }
  }
  return {false, Value::null()};
}

void GroupInterceptor::AddInterceptor(
    std::unique_ptr<ModuleInterceptor> interceptor) {
  interceptors_.push_back(std::move(interceptor));
}

void GroupInterceptor::SetTemplateUrl(const std::string& url) {
  for (const auto& interceptor : interceptors_) {
    interceptor->SetTemplateUrl(url);
  }
}

}  // namespace piper
}  // namespace lynx
