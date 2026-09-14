// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/javascript_execution_callback_android.h"

#include "core/base/android/jni_helper.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxJavaScriptExecutionCallback_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxJavaScriptExecutionCallback_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxJavaScriptExecutionCallback(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni

namespace shell {

void DispatchJavaScriptExecutionResult(
    base::android::ScopedGlobalJavaRef<jobject> callback, int status,
    std::string stack) {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto java_stack =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, stack);
  Java_LynxJavaScriptExecutionCallback_onResult(
      env, callback.Get(), static_cast<jint>(status), java_stack.Get());
}

}  // namespace shell
}  // namespace lynx
