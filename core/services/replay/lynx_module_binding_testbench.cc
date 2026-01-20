// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/lynx_module_binding_testbench.h"

#include <memory>
#include <utility>

namespace lynx {
namespace runtime {
namespace js {
LynxJSIModuleBindingTestBench::LynxJSIModuleBindingTestBench(
    const LynxModuleProviderFunction &moduleProvider)
    : moduleProvider_(moduleProvider) {}

void LynxJSIModuleBindingTestBench::setLynxModuleManagerPtr(
    const LynxJSIModuleBindingPtr moduleProvider) {
  moduleBindingPtrLynx_ = moduleProvider;
}

LynxJSIModuleBindingPtr
LynxJSIModuleBindingTestBench::getLynxModuleManagerPtr() {
  return moduleBindingPtrLynx_;
}

Value LynxJSIModuleBindingTestBench::get(Runtime *rt, const PropNameID &prop) {
  Scope scope(*rt);
  std::string moduleName = prop.utf8(*rt);
  std::shared_ptr<LynxModule> module = nullptr;
  if (lynxModuleSet.find(moduleName) != lynxModuleSet.end()) {
    return moduleBindingPtrLynx_->get(rt, prop);
  } else {
    module = moduleProvider_(moduleName);
  }

  if (module == nullptr) {
    return Value::null();
  }
  return Object::createFromHostObject(*rt, std::move(module));
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
