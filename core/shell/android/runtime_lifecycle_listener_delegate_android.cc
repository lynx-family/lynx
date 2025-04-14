// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "lynx/core/shell/android/runtime_lifecycle_listener_delegate_android.h"

#include "base/include/log/logging.h"
#include "lynx/core/build/gen/RuntimeLifecycleListenerDelegate_jni.h"

namespace lynx {
namespace shell {

RuntimeLifecycleListenerDelegateAndroid::
    RuntimeLifecycleListenerDelegateAndroid(JNIEnv* env, jobject delegate,
                                            jint type)
    : RuntimeLifecycleListenerDelegate(
          type == 0 ? RuntimeLifecycleListenerDelegate::DelegateType::PART
                    : RuntimeLifecycleListenerDelegate::DelegateType::FULL),
      impl_(env, delegate) {}

void RuntimeLifecycleListenerDelegateAndroid::OnRuntimeCreate(
    std::shared_ptr<runtime::IVSyncObserver> observer) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_RuntimeLifecycleListenerDelegate_onRuntimeCreate(
      env, impl_.Get(), reinterpret_cast<jlong>(&observer));
}

void RuntimeLifecycleListenerDelegateAndroid::OnRuntimeInit(
    int64_t runtime_id) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_RuntimeLifecycleListenerDelegate_onRuntimeInit(
      env, impl_.Get(), reinterpret_cast<jlong>(runtime_id));
}

void RuntimeLifecycleListenerDelegateAndroid::OnAppEnterForeground() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_RuntimeLifecycleListenerDelegate_onAppEnterForeground(env, impl_.Get());
}
void RuntimeLifecycleListenerDelegateAndroid::OnAppEnterBackground() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_RuntimeLifecycleListenerDelegate_onAppEnterBackground(env, impl_.Get());
}

void RuntimeLifecycleListenerDelegateAndroid::OnRuntimeAttach(
    Napi::Env current_napi_env) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_RuntimeLifecycleListenerDelegate_onRuntimeAttach(
      env, impl_.Get(),
      reinterpret_cast<jlong>(static_cast<napi_env>(current_napi_env)));
}

void RuntimeLifecycleListenerDelegateAndroid::OnRuntimeDetach() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_RuntimeLifecycleListenerDelegate_onRuntimeDetach(env, impl_.Get());
}

void RuntimeLifecycleListenerDelegateAndroid::RegisterJNI(JNIEnv* env) {
  RegisterNativesImpl(env);
}

}  // namespace shell
}  // namespace lynx
