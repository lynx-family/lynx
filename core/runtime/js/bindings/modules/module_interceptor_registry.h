// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INTERCEPTOR_REGISTRY_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INTERCEPTOR_REGISTRY_H_

#include <memory>

namespace lynx {
namespace runtime {
namespace js {

class GroupInterceptor;
class ModuleInterceptor;

/**
 * Process-wide interceptors added to the GroupInterceptor of every module
 * manager initialized after registration, ahead of the built-in ones.
 */
class ModuleInterceptorRegistry {
 public:
  using Creator = std::unique_ptr<ModuleInterceptor> (*)();

  // Thread-safe. Registering the same creator again has no effect.
  static void RegisterLeading(Creator creator);
  // Returns `group` with the registered interceptors, creating it if needed.
  static std::shared_ptr<GroupInterceptor> Install(
      std::shared_ptr<GroupInterceptor> group);
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INTERCEPTOR_REGISTRY_H_
