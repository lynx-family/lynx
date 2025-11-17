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

void* GetSharedObjectHandler(const char* so_name) {
  if (strcmp(so_name, kAceNdkSoName) == 0) {
    static void* shared_object_handler = nullptr;
    static std::once_flag once_flag;
    std::call_once(once_flag, []() {
      void* handle = dlopen(kAceNdkSoName, RTLD_NOW | RTLD_LOCAL);
      shared_object_handler = handle;
    });
    return shared_object_handler;
  }

  if (strcmp(so_name, kNativeDrawingSoName) == 0) {
    static void* native_drawing_object_handler = nullptr;
    static std::once_flag native_drawing_once_flag;
    std::call_once(native_drawing_once_flag, []() {
      void* native_drawing_handle =
          dlopen(kNativeDrawingSoName, RTLD_NOW | RTLD_LOCAL);
      native_drawing_object_handler = native_drawing_handle;
    });
    return native_drawing_object_handler;
  }

  return nullptr;
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

HarmonyCompatFunctionsHandler* GetHarmonyCompatFunctionsHandler() {
  static std::once_flag once_flag;
  static HarmonyCompatFunctionsHandler* harmony_compat_handler = nullptr;
  std::call_once(once_flag, []() {
    void* handle = GetSharedObjectHandler(kAceNdkSoName);
    if (handle == nullptr) {
      return;
    }
    auto* func_handler = DlSymAllSymbolNeeded(handle);
    if (func_handler != nullptr) {
      harmony_compat_handler = func_handler;
    } else {
      LOGW("Failed to load ace_ndk symbol");
    }
  });
  return harmony_compat_handler;
}

// get native drawing functions
NativeDrawingFunctionsHandler* DlSymAllNativeDrawingSymbolNeeded(
    void* handler) {
  NativeDrawingFunctionsHandler* funcs = new NativeDrawingFunctionsHandler();
  bool success = true;
#define X(ret, name, args, symbol)                     \
  funcs->name = (ret(*) args)dlsym(handler, symbol);   \
  if (!funcs->name) {                                  \
    LOGE("Failed to load harmony symbol::" << symbol); \
    success = false;                                   \
  }

  NATIVE_DRAWING_FUNCTIONS
#undef X
  if (!success) {
    delete funcs;
    return nullptr;
  }
  return funcs;
}

NativeDrawingFunctionsHandler* GetNativeDrawingFunctionsHandler() {
  static std::once_flag once_flag;
  static NativeDrawingFunctionsHandler* native_drawing_handler = nullptr;
  std::call_once(once_flag, []() {
    void* handler = GetSharedObjectHandler(kNativeDrawingSoName);
    if (handler == nullptr) {
      return;
    }
    auto* func_handler = DlSymAllNativeDrawingSymbolNeeded(handler);
    if (func_handler != nullptr) {
      native_drawing_handler = func_handler;
    } else {
      LOGW("Failed to load native_drawing symbol");
    }
  });
  return native_drawing_handler;
}

}  // namespace harmony
}  // namespace base
}  // namespace lynx
