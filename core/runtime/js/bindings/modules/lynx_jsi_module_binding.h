// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_JSI_MODULE_BINDING_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_JSI_MODULE_BINDING_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/include/vector.h"
#include "core/runtime/js/bindings/modules/lynx_module.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace runtime {
namespace js {
/**
 * Represents the JavaScript binding for the LynxModule system.
 */
class LynxJSIModuleBinding : public HostObject {
 public:
  struct LookupFallback {
    Runtime& runtime;
    Value& result;
  };
  using Provider = std::function<std::shared_ptr<LynxModule>(const std::string&,
                                                             LookupFallback*)>;

  explicit LynxJSIModuleBinding(
      const LynxModuleProviderFunction& moduleProvider)
      : LynxJSIModuleBinding(
            [moduleProvider](const std::string& name, LookupFallback*) {
              return moduleProvider(name);
            }) {}
  explicit LynxJSIModuleBinding(std::nullptr_t) {}
  explicit LynxJSIModuleBinding(Provider moduleProvider);
  ~LynxJSIModuleBinding() override = default;

  Value get(Runtime* rt, const PropNameID& name) override;
  std::shared_ptr<LynxModule> GetModule(const std::string& name);

 private:
  Provider moduleProvider_;
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_JSI_MODULE_BINDING_H_
