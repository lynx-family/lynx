// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/harmony/harmony_function_loader.h"

#include <dlfcn.h>

#include <mutex>

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace harmony {

namespace {
void* TryGetSharedObjectHandler() {
  static void* shared_object_handler = nullptr;
  static std::once_flag once_flag;
  std::call_once(once_flag, []() {
    void* handle = dlopen("libace_ndk.z.so", RTLD_NOW | RTLD_LOCAL);
    shared_object_handler = handle;
  });
  return shared_object_handler;
}

HarmonyCompatFunctionsHandler* DlSymAllSymbolNeeded(void* handler) {
  HarmonyCompatFunctionsHandler* funcs = new HarmonyCompatFunctionsHandler();
  bool success = true;
#define X(ret, name, args, symbol)                     \
  funcs->name = (ret(*) args)dlsym(handler, symbol);   \
  if (!funcs->name) {                                  \
    LOGE("Failed to load harmony symbol::" << symbol); \
    success = false;                                   \
  }

  HARMONY_COMPAT_FUNCTIONS
#undef X
  if (!success) {
    delete funcs;
    return nullptr;
  }
  return funcs;
}
}  // namespace

HarmonyCompatFunctionsHandler* GetHarmonyCompatFunctionsHandler() {
  void* handle = TryGetSharedObjectHandler();
  if (handle == nullptr) {
    return nullptr;
  }
  static std::once_flag once_flag;
  static HarmonyCompatFunctionsHandler* harmony_compat_handler = nullptr;
  std::call_once(once_flag, [&]() {
    auto* func_handler = DlSymAllSymbolNeeded(handle);
    if (func_handler != nullptr) {
      harmony_compat_handler = func_handler;
    } else {
      LOGW("Failed to load ace_ndk symbol");
    }
  });
  return harmony_compat_handler;
}

}  // namespace harmony
}  // namespace base
}  // namespace lynx
