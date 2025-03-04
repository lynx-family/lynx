// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/callback_impl.h"

#include <string>
#include <utility>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/CallbackImpl_jni.h"
#include "core/runtime/bindings/jsi/interceptor/android/request_interceptor_android.h"
#include "core/runtime/bindings/jsi/interceptor/network_monitor.h"
#include "core/runtime/bindings/jsi/modules/android/method_invoker.h"
#include "core/services/recorder/recorder_controller.h"

void Invoke(JNIEnv* env, jobject jcaller, jlong nativePtr, jobject array) {
  auto callbackWeakImpl =
      reinterpret_cast<std::weak_ptr<lynx::piper::ModuleCallbackAndroid>*>(
          nativePtr);
  auto callbackImpl = callbackWeakImpl->lock();
  if (callbackImpl == nullptr) {
    LOGE(
        "LynxModule, callback_impl, nativeInvoke, "
        "callbackImpl.lock() is a nullptr");
    return;
  }
  callbackImpl->setArguments(
      lynx::base::android::ScopedGlobalJavaRef<jobject>(env, array));
  auto methodInvoker = callbackImpl->methodInvoker.lock();
  if (methodInvoker != nullptr) {
    methodInvoker->InvokeCallback(callbackImpl);
  } else {
    LOGE(
        "LynxModule, callback_impl, nativeInvoke, "
        "callbackImpl->methodInvoker.lock() is a nullptr");
  }
}
void ReleaseNativePtr(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  auto callbackWeakImpl =
      reinterpret_cast<std::weak_ptr<lynx::piper::ModuleCallbackAndroid>*>(
          nativePtr);
  delete callbackWeakImpl;
}

namespace lynx {
namespace piper {

static jclass jniClass;
static jmethodID ctor;

bool ModuleCallbackAndroid::RegisterJNI(JNIEnv* env) {
  jniClass = static_cast<jclass>(
      // NOLINTNEXTLINE
      env->NewGlobalRef(env->FindClass("com/lynx/jsbridge/CallbackImpl")));
  ctor = env->GetMethodID(jniClass, "<init>", "(J)V");
  return RegisterNativesImpl(env);
}

ModuleCallbackAndroid::CallbackPair ModuleCallbackAndroid::createCallbackImpl(
    int64_t callback_id, std::shared_ptr<MethodInvoker> invoker,
    ModuleCallbackType type) {
  std::shared_ptr<ModuleCallbackAndroid> callback;
  // TODO(huzhanbo.luc): move this into request_interceptor
  switch (type) {
    case ModuleCallbackType::Base:
      callback = std::make_shared<ModuleCallbackAndroid>(callback_id, invoker);
      break;
    case ModuleCallbackType::Request:
    case ModuleCallbackType::Fetch:
      callback =
          std::make_shared<ModuleCallbackRequest>(callback_id, invoker, type);
      break;
  }
  auto weakCallback = new std::weak_ptr<ModuleCallbackAndroid>(callback);
  auto nativePtr = reinterpret_cast<jlong>(weakCallback);
  JNIEnv* env = base::android::AttachCurrentThread();
  jobject javaObj = env->NewObject(jniClass, ctor, nativePtr);
  ModuleCallbackAndroid::CallbackPair pair = std::make_pair(
      callback, base::android::ScopedGlobalJavaRef<jobject>(env, javaObj));
  env->DeleteLocalRef(javaObj);
  return pair;
}

void ModuleCallbackAndroid::Invoke(Runtime* runtime,
                                   ModuleCallbackFunctionHolder* holder) {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (runtime == nullptr) {
    LOGE("lynx ModuleCallback has null runtime.");
    if (timing_collector_ != nullptr) {
      timing_collector_->OnErrorOccurred(
          NativeModuleStatusCode::PARAMETER_ERROR);
    }
    return;
  }
  piper::Scope scope(*runtime);

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, "JNIValueToJSValue");
  uint64_t convert_params_start = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_convert_params_start",
      [first_arg = timing_collector_->GetFirstArg(),
       convert_params_start](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg", first_arg);
        ctx.event()->add_debug_annotations(
            "timestamp", std::to_string(convert_params_start));
      });
  auto arr = jsArrayFromJavaOnlyArray(env, arguments.Get(), runtime);
  if (!arr) {
    LOGE(
        "invoke JSB callback fail! Reason: Transfer JAVA value to "
        "js value fail.");
    if (timing_collector_ != nullptr) {
      timing_collector_->OnErrorOccurred(
          NativeModuleStatusCode::PARAMETER_ERROR);
    }
    return;
  }
  auto size = (*arr).length(*runtime);
  if (!size) {
    if (timing_collector_ != nullptr) {
      timing_collector_->OnErrorOccurred(NativeModuleStatusCode::FAILURE);
    }
    return;
  }
  piper::Value values[*size];
  for (size_t index = 0; index < *size; index++) {
    auto item = (*arr).getValueAtIndex(*runtime, index);
    if (!item) {
      if (timing_collector_ != nullptr) {
        timing_collector_->OnErrorOccurred(
            NativeModuleStatusCode::PARAMETER_ERROR);
      }
      return;
    }
    values[index] = std::move(*item);
  }
  uint64_t convert_params_end = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_convert_params_end",
      [first_arg = timing_collector_->GetFirstArg(), convert_params_start,
       convert_params_end](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg", first_arg);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(convert_params_end));
        ctx.event()->add_debug_annotations(
            "jsb_callback_convert_params",
            std::to_string(convert_params_end - convert_params_start));
      });
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "InvokeCallback");
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_invoke_start",
      [first_arg = timing_collector_->GetFirstArg(),
       convert_params_end](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg", first_arg);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(convert_params_end));
      });
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::NativeModuleRecorder::GetInstance().RecordCallback(
      module_name_.c_str(), method_name_.c_str(), values[0], runtime,
      callback_id(), record_id_);
#endif
  holder->function_.call(*runtime, values, *size);
  if (timing_collector_ != nullptr) {
    timing_collector_->EndCallbackInvoke(
        (convert_params_end - convert_params_start), convert_params_end);
    network::ReportRequestSuccessIfNecessary(timing_collector_, this);
  }
}

ModuleCallbackAndroid::ModuleCallbackAndroid(
    int64_t callback_id, std::shared_ptr<MethodInvoker> invoker)
    : ModuleCallback(callback_id), methodInvoker(invoker) {}
}  // namespace piper
}  // namespace lynx
