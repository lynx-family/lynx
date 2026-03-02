// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/lynx_engine_wrapper_android.h"

#include "core/shell/lynx_engine_wrapper.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxEngine_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxEngine_register_jni.h"

namespace lynx {
namespace jni {

bool RegisterJNIForLynxEngine(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynx

jlong Create(JNIEnv* env, jobject jcaller) {
  auto* lynx_engine_wrapper = new lynx::shell::LynxEngineWrapper();
  return reinterpret_cast<int64_t>(lynx_engine_wrapper);
}

jlong DetachEngine(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  // TODO
  return 0;
}

void DestroyEngine(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  lynx::shell::LynxEngineWrapper* engine_wrapper =
      reinterpret_cast<lynx::shell::LynxEngineWrapper*>(nativePtr);
  engine_wrapper->DestroyEngine();
  delete engine_wrapper;
}
