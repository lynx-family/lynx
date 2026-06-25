// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dlfcn.h>

#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"

namespace lynx {
namespace devtool {
namespace {

using RegisterRTSInspectorManagerFactoryFn = void (*)(JSDebugHelper*);
constexpr const char* kRTSInspectorManagerFactorySymbolName =
    "LynxRegisterRTSInspectorManagerFactoryImpl";

RegisterRTSInspectorManagerFactoryFn ResolveRTSInspectorManagerFactory(
    void* handle, const char* symbol_name) {
  if (handle == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<RegisterRTSInspectorManagerFactoryFn>(
      dlsym(handle, symbol_name));
}

}  // namespace

void JSDebugHelper::RegisterRTSInspectorManagerCreatorByPlatform() {
  dlerror();
  auto register_factory = ResolveRTSInspectorManagerFactory(
      RTLD_DEFAULT, kRTSInspectorManagerFactorySymbolName);
  (void)dlerror();
  if (register_factory != nullptr) {
    register_factory(this);
  }
}

}  // namespace devtool
}  // namespace lynx
