// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INVOKE_SCOPE_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INVOKE_SCOPE_H_

#include <memory>

#include "core/public/jsb/lynx_module_callback.h"

namespace lynx {
namespace runtime {
namespace js {

class GroupInterceptor;
class ModuleCallContext;
class ModuleDelegate;

// Runs cleanup on every return path, including partial argument conversion.
// All referenced locals are declared before this guard. No converted arguments
// are exposed to cleanup hooks after the native module takes ownership.
struct InvokeMethodEndScope {
  GroupInterceptor* interceptor;
  ModuleCallContext* context;
  const std::shared_ptr<ModuleDelegate>& delegate;
  const CallbackMap& callbacks;
  bool args_converted = false;

  ~InvokeMethodEndScope();
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INVOKE_SCOPE_H_
