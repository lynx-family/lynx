// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_CALL_CONTEXT_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_CALL_CONTEXT_H_

namespace lynx::runtime::js {
class GroupInterceptor;
class ModuleInterceptor;

// One method invocation and all callbacks it registers share this context.
// At most one interceptor supplies a derived context, whose data is accessed
// only by that interceptor on the JS thread. It must not borrow arguments,
// runtime objects or other data from the invocation stack.
class ModuleCallContext {
 public:
  virtual ~ModuleCallContext() = default;

  // During interception, the interceptor currently handling the call;
  // afterwards, the one that resolved it, or null for a native call.
  // Set before dispatch so synchronous callbacks use the same boundary.
  const ModuleInterceptor* resolver() const { return resolver_; }

 private:
  friend class GroupInterceptor;
  const ModuleInterceptor* resolver_ = nullptr;
};

}  // namespace lynx::runtime::js

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_CALL_CONTEXT_H_
