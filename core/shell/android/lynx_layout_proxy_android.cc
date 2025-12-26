// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/android/lynx_layout_proxy_android.h"

#include <utility>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/shell/lynx_shell.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxLayoutProxy_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxLayoutProxy_register_jni.h"

using lynx::base::android::ScopedGlobalJavaRef;

namespace lynx {

namespace jni {

bool RegisterJNIForLynxLayoutProxy(JNIEnv *env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

static jlong Create(JNIEnv *env, jobject jcaller, jlong nativePtr) {
  auto *shell = reinterpret_cast<lynx::shell::LynxShell *>(nativePtr);
  lynx::shell::LynxLayoutProxyAndroid *layout_proxy =
      new lynx::shell::LynxLayoutProxyAndroid(shell->GetLayoutActor());
  return reinterpret_cast<jlong>(layout_proxy);
}

static void Release(JNIEnv *env, jobject jcaller, jlong nativePtr) {
  delete reinterpret_cast<lynx::shell::LynxLayoutProxyAndroid *>(nativePtr);
}

static void RunOnLayoutThread(JNIEnv *env, jobject jcaller, jlong nativePtr,
                              jobject java_runnable) {
  auto *layout_proxy =
      reinterpret_cast<lynx::shell::LynxLayoutProxyAndroid *>(nativePtr);
  if (layout_proxy != nullptr) {
    layout_proxy->RunOnLayoutThread(env, jcaller, nativePtr, java_runnable);
  }
}

namespace lynx {

namespace shell {

void LynxLayoutProxyAndroid::RunOnLayoutThread(JNIEnv *env, jobject jcaller,
                                               jlong nativePtr,
                                               jobject java_runnable) {
  lynx::base::android::ScopedGlobalJavaRef<jobject> runnable(env,
                                                             java_runnable);
  if (runnable.Get() == nullptr) {
    return;
  }

  layout_proxy_->DispatchTaskToLynxLayout([runnable = std::move(runnable)]() {
    JNIEnv *env = lynx::base::android::AttachCurrentThread();
    jclass runnable_class = env->GetObjectClass(runnable.Get());
    if (runnable_class == nullptr) {
      return;
    }
    jmethodID run_method_id = env->GetMethodID(runnable_class, "run", "()V");
    if (!run_method_id) {
      return;
    }
    env->CallVoidMethod(runnable.Get(), run_method_id);
  });
}

}  // namespace shell
}  // namespace lynx
