// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/runtime_lifecycle_listener_delegate_android.h"

#include "base/include/log/logging.h"
#include "base/include/platform/android/jni_convert_helper.h"
#include "platform/android/lynx_android/src/main/jni/gen/RuntimeLifecycleListenerDelegate_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/RuntimeLifecycleListenerDelegate_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForRuntimeLifecycleListenerDelegate(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace shell {

RuntimeLifecycleListenerDelegateAndroid::
    RuntimeLifecycleListenerDelegateAndroid(JNIEnv* env, jobject delegate)
    : RuntimeLifecycleListenerDelegate(
          RuntimeLifecycleListenerDelegate::DelegateType::PART),
      impl_(env, delegate) {}

void RuntimeLifecycleListenerDelegateAndroid::OnRuntimeAttach(
    void* current_napi_env, const char* runtime_type) {
  JNIEnv* env = base::android::AttachCurrentThread();
  runtime_type = runtime_type ? runtime_type : "";
  auto java_runtime_type =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, runtime_type);
  Java_RuntimeLifecycleListenerDelegate_onRuntimeAttach(
      env, impl_.Get(),
      reinterpret_cast<jlong>(static_cast<napi_env>(current_napi_env)),
      java_runtime_type.Get());
}

void RuntimeLifecycleListenerDelegateAndroid::OnRuntimeDetach() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_RuntimeLifecycleListenerDelegate_onRuntimeDetach(env, impl_.Get());
}

}  // namespace shell
}  // namespace lynx
