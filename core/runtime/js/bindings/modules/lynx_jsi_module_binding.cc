// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/lynx_jsi_module_binding.h"

#include <utility>

namespace lynx {
namespace runtime {
namespace js {
/**
 * Public API to install the LynxModule system.
 */
LynxJSIModuleBinding::LynxJSIModuleBinding(Provider moduleProvider)
    : moduleProvider_(std::move(moduleProvider)) {}

Value LynxJSIModuleBinding::get(Runtime* rt, const PropNameID& prop) {
  Scope scope(*rt);
  std::string moduleName = prop.utf8(*rt);
  auto replacement = Value::null();
  LookupFallback fallback{*rt, replacement};
  std::shared_ptr<LynxModule> module = moduleProvider_(moduleName, &fallback);
  if (module == nullptr) {
    return replacement;
  }
  return Object::createFromHostObject(*rt, std::move(module));
}

std::shared_ptr<LynxModule> LynxJSIModuleBinding::GetModule(
    const std::string& name) {
  return moduleProvider_(name, nullptr);
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
