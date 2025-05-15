// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/js_proxy_android.h"

#include <unordered_map>
#include <utility>

#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/shell/android/lynx_runtime_wrapper_android.h"
#include "core/shell/android/runtime_lifecycle_listener_delegate_android.h"
#include "core/shell/lynx_runtime_proxy_impl.h"
#include "core/shell/lynx_shell.h"
#include "core/value_wrapper/android/value_impl_android.h"
#include "core/value_wrapper/value_impl_piper.h"
#include "core/value_wrapper/value_wrapper_utils.h"
#include "platform/android/lynx_android/src/main/jni/gen/JSProxy_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/JSProxy_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForJSProxy(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynx

using lynx::base::android::AttachCurrentThread;
using lynx::base::android::JNIConvertHelper;
using lynx::base::android::ScopedGlobalJavaRef;
using lynx::shell::JSProxyAndroid;

#define JS_PROXY reinterpret_cast<JSProxyAndroid*>(ptr)

jlong Create(JNIEnv* env, jobject jcaller, jlong ptr,
             jstring js_group_thread_name_jstring) {
  auto* shell = reinterpret_cast<lynx::shell::LynxShell*>(ptr);
  auto runtime_actor = shell->GetRuntimeActor();
  auto js_group_thread_name =
      JNIConvertHelper::ConvertToString(env, js_group_thread_name_jstring);
  auto proxy =
      new JSProxyAndroid(runtime_actor, runtime_actor->Impl()->GetRuntimeId(),
                         env, jcaller, js_group_thread_name, false);
  Java_JSProxy_setRuntimeId(env, jcaller, proxy->GetId());
  return reinterpret_cast<jlong>(proxy);
}

// In LynxBackgroundRuntime standalone, runtime_actor is created outside
// of LynxShell. We should get it from `LynxRuntimeWrapperAndroid`.
jlong CreateWithRuntimeActor(JNIEnv* env, jobject jcaller, jlong ptr,

                             jstring js_group_thread_name_jstring) {
  auto* shell = reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid*>(ptr);
  auto runtime_actor = shell->GetRuntimeActor();
  auto js_group_thread_name =
      JNIConvertHelper::ConvertToString(env, js_group_thread_name_jstring);
  auto proxy =
      new JSProxyAndroid(runtime_actor, runtime_actor->Impl()->GetRuntimeId(),
                         env, jcaller, js_group_thread_name, true);
  Java_JSProxy_setRuntimeId(env, jcaller, proxy->GetId());
  return reinterpret_cast<jlong>(proxy);
}

void Destroy(JNIEnv* env, jobject jcaller, jlong ptr,
             jstring js_group_thread_name_jstring) {
  lynx::base::TaskRunnerManufactor::GetJSRunner(
      JNIConvertHelper::ConvertToString(env, js_group_thread_name_jstring))
      ->PostTask([ptr]() { delete JS_PROXY; });
}

void CallJSFunction(JNIEnv* env, jobject jcaller, jlong ptr, jstring module,
                    jstring method, jlong args_id) {
  JS_PROXY->CallJSFunctionByArgsId(
      JNIConvertHelper::ConvertToString(env, module),
      JNIConvertHelper::ConvertToString(env, method), args_id);
}

void CallIntersectionObserver(JNIEnv* env, jobject jcaller, jlong ptr,
                              jint observer_id, jint callback_id,
                              jlong args_id) {
  JS_PROXY->CallJSIntersectionObserverByArgsId(observer_id, callback_id,
                                               args_id);
}

void CallJSApiCallbackWithValue(JNIEnv* env, jobject jcaller, jlong ptr,
                                jint callback_id, jlong args_id) {
  JS_PROXY->CallJSApiCallbackWithValueByArgsId(callback_id, args_id);
}

void EvaluateScript(JNIEnv* env, jclass jcaller, jlong ptr, jstring url,
                    jbyteArray data, jint callback_id) {
  JS_PROXY->EvaluateScript(JNIConvertHelper::ConvertToString(env, url),
                           JNIConvertHelper::ConvertToString(env, data),
                           callback_id);
}

static void RejectDynamicComponentLoad(JNIEnv* env, jclass jcaller, jlong ptr,
                                       jstring url, jint callback_id,
                                       jint err_code, jstring err_msg) {
  JS_PROXY->RejectDynamicComponentLoad(
      JNIConvertHelper::ConvertToString(env, url), callback_id, err_code,
      JNIConvertHelper::ConvertToString(env, err_msg));
}

void RunOnJSThread(JNIEnv* env, jclass jcaller, jlong ptr, jobject runnable) {
  auto global_runnable_scoped_ref = ScopedGlobalJavaRef<jobject>{env, runnable};
  if (!global_runnable_scoped_ref.Get()) {
    return;
  }
  JS_PROXY->RunOnJSThread([global_runnable_scoped_ref =
                               std::move(global_runnable_scoped_ref)]() {
    auto* env = AttachCurrentThread();
    auto global_runnable = global_runnable_scoped_ref.Get();
    jclass runnable_class = env->GetObjectClass(global_runnable);
    if (!runnable_class) {
      return;
    }
    jmethodID run_method_id = env->GetMethodID(runnable_class, "run", "()V");
    if (!run_method_id) {
      return;
    }
    env->CallVoidMethod(global_runnable, run_method_id);
  });
}

void AddLifecycleListener(JNIEnv* env, jobject jcaller, jlong ptr,
                          jobject delegate) {
  JS_PROXY->AddLifecycleListener(
      std::make_unique<lynx::shell::RuntimeLifecycleListenerDelegateAndroid>(
          env, delegate));
}

namespace lynx {
namespace shell {

std::unique_ptr<pub::Value> JSProxyAndroid::GetArgs(
    std::shared_ptr<base::android::ScopedWeakGlobalJavaRef<jobject>> jni_object,
    long args_id, bool is_array) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(*jni_object);
  if (local_ref.IsNull()) {
    return nullptr;
  }
  auto args = Java_JSProxy_getArgs(env, local_ref.Get(), args_id);
  if (args.IsNull()) {
    return nullptr;
  }

  if (is_array) {
    return std::make_unique<pub::ValueImplAndroid>(
        std::make_shared<base::android::JavaOnlyArray>(env, std::move(args)));
  } else {
    return std::make_unique<pub::ValueImplAndroid>(
        std::make_shared<base::android::JavaOnlyMap>(env, std::move(args)));
  }
}

void JSProxyAndroid::CallJSFunctionByArgsId(std::string module_id,
                                            std::string method_id,
                                            long args_id) {
  LynxRuntimeProxyImpl::CallJSFunction(
      module_id, method_id,
      [args_id,
       jni_object = jni_object_]() mutable -> std::unique_ptr<pub::Value> {
        return GetArgs(jni_object, args_id, true);
      });
}

void JSProxyAndroid::CallJSIntersectionObserverByArgsId(int32_t observer_id,
                                                        int32_t callback_id,
                                                        long args_id) {
  LynxRuntimeProxyImpl::CallJSIntersectionObserver(
      observer_id, callback_id,
      [args_id,
       jni_object = jni_object_]() mutable -> std::unique_ptr<pub::Value> {
        return GetArgs(jni_object, args_id, false);
      });
}

void JSProxyAndroid::CallJSApiCallbackWithValueByArgsId(int32_t callback_id,
                                                        long args_id) {
  LynxRuntimeProxyImpl::CallJSApiCallbackWithValue(
      callback_id,
      [args_id,
       jni_object = jni_object_]() mutable -> std::unique_ptr<pub::Value> {
        return GetArgs(jni_object, args_id, false);
      });
}

void JSProxyAndroid::RunOnJSThread(base::MoveOnlyClosure<> task) {
  if (!task) {
    return;
  }
  actor_->Act([task = std::move(task)](auto& runtime) { task(); });
}

}  // namespace shell
}  // namespace lynx
