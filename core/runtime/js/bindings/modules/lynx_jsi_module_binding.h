// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_JSI_MODULE_BINDING_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_JSI_MODULE_BINDING_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/vector.h"
#include "core/runtime/js/bindings/modules/lynx_module.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace piper {

/**
 * Represents the JavaScript binding for the LynxModule system.
 */
class LynxJSIModuleBinding : public piper::HostObject {
 public:
  explicit LynxJSIModuleBinding(
      const LynxModuleProviderFunction& moduleProvider);
  ~LynxJSIModuleBinding() override = default;

  piper::Value get(Runtime* rt, const PropNameID& name) override;
  std::shared_ptr<LynxModule> GetModule(const std::string& name);

 private:
  LynxModuleProviderFunction moduleProvider_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_JSI_MODULE_BINDING_H_
