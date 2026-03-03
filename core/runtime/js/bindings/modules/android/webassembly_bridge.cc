// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/jsi/quickjs/quickjs_context_wrapper.h"
#include "platform/android/lynx_android/src/main/jni/gen/WebAssemblyBridge_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/WebAssemblyBridge_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForWebAssemblyBridge(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace runtime {
namespace js {
void RegisterFunction(int64_t func_ptr) {
  LOGI("Setting webassembly register function pointer from JAVA");
  QuickjsContextWrapper::register_wasm_func_ =
      reinterpret_cast<RegisterWasmFuncType>(func_ptr);
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx

// static
void InitWasm(JNIEnv* env, jclass jcaller, jlong func_ptr) {
  lynx::runtime::js::RegisterFunction(func_ptr);
}
