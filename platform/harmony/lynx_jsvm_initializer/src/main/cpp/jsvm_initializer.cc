// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/harmony/lynx_jsvm_initializer/src/main/cpp/jsvm_initializer.h"

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>
#include <dlfcn.h>

#include <mutex>

namespace {
using JSVMInitFn = JSVM_Status (*)(const JSVM_InitOptions *options);
std::once_flag jsvm_init_once;
void *jsvm_lib_handle = nullptr;
JSVMInitFn jsvm_init_fn = nullptr;
JSVM_Status jsvm_init_status = static_cast<JSVM_Status>(1);

JSVMInitFn ResolveJSVMInitFn() {
  if (jsvm_init_fn) {
    return jsvm_init_fn;
  }

  if (!jsvm_lib_handle) {
    jsvm_lib_handle = dlopen("/system/lib64/ndk/libjsvm.so", RTLD_LAZY);
  }
  if (!jsvm_lib_handle) {
    return nullptr;
  }

  jsvm_init_fn =
      reinterpret_cast<JSVMInitFn>(dlsym(jsvm_lib_handle, "OH_JSVM_Init"));
  return jsvm_init_fn;
}
}  // namespace

__attribute__((visibility("default"))) JSVM_Status Lynx_JSVM_Common_Init(
    const JSVM_InitOptions *options) {
  std::call_once(jsvm_init_once, [options]() {
    auto init_fn = ResolveJSVMInitFn();
    if (!init_fn) {
      return;
    }
    jsvm_init_status = init_fn(options);
  });
  return jsvm_init_status;
}
