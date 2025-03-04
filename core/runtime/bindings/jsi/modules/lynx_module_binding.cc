// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/lynx_module_binding.h"

#include <utility>

namespace lynx {
namespace piper {

/**
 * Public API to install the LynxModule system.
 */
LynxModuleBinding::LynxModuleBinding(
    const LynxModuleProviderFunction& moduleProvider)
    : moduleProvider_(moduleProvider) {}

piper::Value LynxModuleBinding::get(Runtime* rt, const PropNameID& prop) {
  piper::Scope scope(*rt);
  std::string moduleName = prop.utf8(*rt);
  std::shared_ptr<LynxModule> module = moduleProvider_(moduleName);
  if (module == nullptr) {
    return piper::Value::null();
  }
  return piper::Object::createFromHostObject(*rt, std::move(module));
}

std::shared_ptr<LynxModule> LynxModuleBinding::GetModule(
    const std::string& name) {
  return moduleProvider_(name);
}

}  // namespace piper
}  // namespace lynx
