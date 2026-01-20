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
LynxJSIModuleBinding::LynxJSIModuleBinding(
    const LynxModuleProviderFunction& moduleProvider)
    : moduleProvider_(moduleProvider) {}

Value LynxJSIModuleBinding::get(Runtime* rt, const PropNameID& prop) {
  Scope scope(*rt);
  std::string moduleName = prop.utf8(*rt);
  std::shared_ptr<LynxModule> module = moduleProvider_(moduleName);
  if (module == nullptr) {
    return Value::null();
  }
  return Object::createFromHostObject(*rt, std::move(module));
}

std::shared_ptr<LynxModule> LynxJSIModuleBinding::GetModule(
    const std::string& name) {
  return moduleProvider_(name);
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
