// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/runtime_lifecycle_listener_delegate_android.h"

#include "base/include/log/logging.h"
#include "base/include/platform/android/jni_convert_helper.h"
#include "core/public/lynx_extension_delegate.h"
#include "core/shell/runtime/bts/bts_runtime.h"
#include "platform/android/lynx_android/src/main/jni/gen/RuntimeLifecycleListenerDelegate_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/RuntimeLifecycleListenerDelegate_register_jni.h"

namespace {
class RuntimeLifecycleExtensionDelegate
    : public lynx::pub::LynxExtensionDelegate {
 public:
  RuntimeLifecycleExtensionDelegate(JNIEnv* env, jobject factory)
      : factory_(env, factory) {}

  void SetRuntimeActor(
      std::shared_ptr<lynx::shell::LynxActor<lynx::shell::BTSRuntime>> actor)
      override {
    if (!actor || actor_.lock() == actor) return;
    actor_ = actor;
    auto* env = lynx::base::android::AttachCurrentThread();
    auto listener = Java_RuntimeLifecycleListenerDelegate_createForExtension(
        env, factory_.Get());
    if (listener.IsNull()) return;
    actor->Act([listener = lynx::base::android::ScopedGlobalJavaRef<jobject>(
                    env, listener.Get())](auto& runtime) {
      runtime->AddLifecycleListener(
          std::make_unique<
              lynx::shell::RuntimeLifecycleListenerDelegateAndroid>(
              lynx::base::android::AttachCurrentThread(), listener.Get(),
              lynx::runtime::RuntimeLifecycleListenerDelegate::DelegateType::
                  FULL));
    });
  }

 private:
  lynx::base::android::ScopedGlobalJavaRef<jobject> factory_;
  std::weak_ptr<lynx::shell::LynxActor<lynx::shell::BTSRuntime>> actor_;
};
}  // namespace

static jlong CreateExtensionDelegate(JNIEnv* env, jclass, jobject factory) {
  return reinterpret_cast<jlong>(
      new RuntimeLifecycleExtensionDelegate(env, factory));
}

static void DestroyExtensionDelegate(JNIEnv*, jclass, jlong delegate) {
  delete reinterpret_cast<RuntimeLifecycleExtensionDelegate*>(delegate);
}

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
    RuntimeLifecycleListenerDelegateAndroid(JNIEnv* env, jobject delegate,
                                            DelegateType type)
    : RuntimeLifecycleListenerDelegate(type), impl_(env, delegate) {}

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
