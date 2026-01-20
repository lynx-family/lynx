// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/android/callback_impl.h"

#include <string>
#include <utility>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/lynx_trace_categories.h"
#include "core/runtime/js/bindings/modules/android/lynx_module_android.h"
#include "core/runtime/js/bindings/modules/android/method_invoker.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/value_wrapper/android/value_impl_android.h"
#include "platform/android/lynx_android/src/main/jni/gen/BlurUtils_register_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/CallbackImpl_jni.h"

void Invoke(JNIEnv* env, jobject jcaller, jlong nativePtr, jobject array) {
  auto weak_callback_android = reinterpret_cast<
      std::weak_ptr<lynx::runtime::js::ModuleCallbackAndroid>*>(nativePtr);
  auto callback_android = weak_callback_android->lock();
  if (!callback_android) {
    LOGE(
        "LynxModule, callback_impl, nativeInvoke, "
        "callbackImpl.lock() is a nullptr");
    return;
  }
  callback_android->Invoke(
      lynx::base::android::ScopedGlobalJavaRef<jobject>(env, array));
}

void ReleaseNativePtr(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  auto weak_callback_android = reinterpret_cast<
      std::weak_ptr<lynx::runtime::js::ModuleCallbackAndroid>*>(nativePtr);
  delete weak_callback_android;
}

namespace lynx {
namespace runtime {
namespace js {
static jclass jniClass;
static jmethodID ctor;

void ModuleCallbackAndroid::Invoke(
    lynx::base::android::ScopedGlobalJavaRef<jobject> args) {
  if (callback_->GetType() == LynxModuleCallback::Type::JSI) {
    std::static_pointer_cast<ModuleCallback>(callback_)->SetCustomArgsConverter(
        [args = std::move(args), converter = std::move(custom_args_converter_)](
            Runtime* runtime,
            ModuleCallback* callback) mutable -> std::unique_ptr<pub::Value> {
          if (converter) {
            return converter(runtime, callback, args);
          }
          auto pub_array = std::make_shared<base::android::JavaOnlyArray>(args);
          return std::make_unique<pub::ValueImplAndroid>(std::move(pub_array));
        });
  } else {
    auto pub_array = std::make_shared<base::android::JavaOnlyArray>(args);
    callback_->SetArgs(
        std::make_unique<pub::ValueImplAndroid>(std::move(pub_array)));
  }
  std::shared_ptr<LynxModuleAndroid> callback_invoker =
      callback_invoker_.lock();
  if (callback_invoker != nullptr) {
    callback_invoker->InvokeCallback(callback_, promise);
  } else {
    LOGE(
        "LynxModule, callback_impl, nativeInvoke, "
        "callbackImpl->callback_invoker_.lock() is a nullptr");
  }
}

ModuleCallbackAndroid::CallbackPair ModuleCallbackAndroid::CreateCallbackImpl(
    std::shared_ptr<LynxModuleCallback> callback,
    std::shared_ptr<LynxModuleAndroid> invoker) {
  std::shared_ptr<ModuleCallbackAndroid> callback_android =
      std::make_shared<ModuleCallbackAndroid>(invoker, std::move(callback));
  auto weak_callback =
      new std::weak_ptr<ModuleCallbackAndroid>(callback_android);
  auto native_ptr = reinterpret_cast<jlong>(weak_callback);
  JNIEnv* env = base::android::AttachCurrentThread();
  jobject javaObj = env->NewObject(jniClass, ctor, native_ptr);
  ModuleCallbackAndroid::CallbackPair pair =
      std::make_pair(callback_android,
                     base::android::ScopedGlobalJavaRef<jobject>(env, javaObj));
  env->DeleteLocalRef(javaObj);
  return pair;
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx

namespace lynx {
namespace jni {
bool RegisterJNIForCallbackImpl(JNIEnv* env) {
  lynx::runtime::js::jniClass = static_cast<jclass>(
      // NOLINTNEXTLINE
      env->NewGlobalRef(env->FindClass("com/lynx/jsbridge/CallbackImpl")));
  lynx::runtime::js::ctor =
      env->GetMethodID(lynx::runtime::js::jniClass, "<init>", "(J)V");
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx
