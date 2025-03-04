// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/lynx_promise_impl.h"

#include "core/base/android/android_jni.h"

namespace lynx {
namespace piper {

static jclass jniClass;
bool LynxPromiseImpl::RegisterJNI(JNIEnv* env) {
  jniClass = static_cast<jclass>(
      // NOLINTNEXTLINE
      env->NewGlobalRef(env->FindClass("com/lynx/jsbridge/PromiseImpl")));
  return true;
}

LynxPromiseImpl::LynxPromiseImpl(
    ModuleCallbackAndroid::CallbackPair resolve_pair,
    ModuleCallbackAndroid::CallbackPair reject_pair)
    : resolveCallback_(resolve_pair.first), rejectCallback_(reject_pair.first) {
  JNIEnv* env = base::android::AttachCurrentThread();
  jmethodID ctor = env->GetMethodID(
      jniClass, "<init>",
      "(Lcom/lynx/react/bridge/Callback;Lcom/lynx/react/bridge/Callback;)V");
  jobject java_obj = env->NewObject(jniClass, ctor, resolve_pair.second.Get(),
                                    reject_pair.second.Get());
  jni_object_.Reset(env, java_obj);
  env->DeleteLocalRef(java_obj);
}

std::shared_ptr<ModuleCallbackAndroid> LynxPromiseImpl::GetReject() {
  return rejectCallback_;
}

std::shared_ptr<ModuleCallbackAndroid> LynxPromiseImpl::GetResolve() {
  return resolveCallback_;
}

jobject LynxPromiseImpl::GetJniObject() { return jni_object_.Get(); }

}  // namespace piper
}  // namespace lynx
