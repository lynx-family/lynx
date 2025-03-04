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
#include "core/base/lynx_trace_categories.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/build/gen/JSProxy_jni.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/shell/android/lynx_runtime_wrapper_android.h"
#include "core/shell/lynx_shell.h"

using lynx::base::android::AttachCurrentThread;
using lynx::base::android::JNIConvertHelper;
using lynx::base::android::ScopedGlobalJavaRef;
using lynx::piper::jsArrayFromJavaOnlyArray;
using lynx::piper::jsObjectFromJavaOnlyMap;
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
  JS_PROXY->CallJSFunction(JNIConvertHelper::ConvertToString(env, module),
                           JNIConvertHelper::ConvertToString(env, method),
                           args_id);
}

void CallIntersectionObserver(JNIEnv* env, jobject jcaller, jlong ptr,
                              jint observer_id, jint callback_id,
                              jlong args_id) {
  JS_PROXY->CallJSIntersectionObserver(observer_id, callback_id, args_id);
}

void CallJSApiCallbackWithValue(JNIEnv* env, jobject jcaller, jlong ptr,
                                jint callback_id, jlong args_id) {
  JS_PROXY->CallJSApiCallbackWithValue(callback_id, args_id);
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

namespace lynx {
namespace shell {

bool JSProxyAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void JSProxyAndroid::CallJSFunction(std::string module_id,
                                    std::string method_id, long args_id) {
  actor_->Act([module_id = std::move(module_id),
               method_id = std::move(method_id), args_id,
               runtime_standalone_mode = runtime_standalone_mode_,
               jni_object = jni_object_,
               instance_id = actor_->GetInstanceId()](auto& runtime) mutable {
    auto task = [&runtime, module_id = std::move(module_id),
                 method_id = std::move(method_id), args_id, jni_object,
                 instance_id]() mutable {
      auto js_runtime = runtime->GetJSRuntime();
      if (js_runtime == nullptr) {
        LOGR("try call js module before js context is ready! module:"
             << module_id << " method:" << method_id << " args_id:" << args_id
             << &runtime);
        return;
      }
      JNIEnv* env = base::android::AttachCurrentThread();
      base::android::ScopedLocalJavaRef<jobject> local_ref(*jni_object);
      if (local_ref.IsNull()) {
        return;
      }
      piper::Scope scope(*js_runtime);
      tasm::timing::LongTaskMonitor::Scope long_task_scope(
          instance_id, tasm::timing::kJSFuncTask,
          tasm::timing::kTaskNameJSProxyCallJSFunction);
      tasm::timing::LongTaskTiming* timing =
          tasm::timing::LongTaskMonitor::Instance()->GetTopTimingPtr();
      if (timing != nullptr) {
        timing->task_info_ = base::AppendString(module_id, ".", method_id);
      }
      auto args = Java_JSProxy_getArgs(env, local_ref.Get(), args_id);
      if (args.IsNull()) {
        return;
      }
      TRACE_EVENT(
          LYNX_TRACE_CATEGORY, "CallJSFunction",
          [&](lynx::perfetto::EventContext ctx) {
            ctx.event()->add_debug_annotations("module_name", module_id);
            ctx.event()->add_debug_annotations("method_name", method_id);
            int size = base::android::JavaOnlyArray::JavaOnlyArrayGetSize(
                env, args.Get());
            if (size > 0) {
              auto type =
                  base::android::JavaOnlyArray::JavaOnlyArrayGetTypeAtIndex(
                      env, args.Get(), 0);
              if (type == base::android::ReadableType::String) {
                auto str =
                    base::android::JavaOnlyArray::JavaOnlyArrayGetStringAtIndex(
                        env, args.Get(), 0);
                ctx.event()->add_debug_annotations("event_name", str);
              }
            }
          });
      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                        "CallJSFunction:JavaOnlyArrayToJSArray");
      auto params = jsArrayFromJavaOnlyArray(AttachCurrentThread(), args.Get(),
                                             js_runtime.get());
      if (!params) {
        js_runtime->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
            "CallJSFunction fail! Reason: Transfer java value to js "
            "value fail."));
        return;
      }
      TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallJSFunction:Fire");
      runtime->CallFunction(module_id, method_id, *params);
    };

    // In LynxBackgroundRuntime standalone, we don't need to postpone
    // GlobalEvent to LoadApp via cache. User can decide the time to
    // run FE code and sending GlobalEvent, so it is user's responsibility
    // to ensure that events arrive after FE code execution.
    if (runtime_standalone_mode) {
      if (runtime->IsRuntimeReady()) {
        runtime->OnErrorOccurred(base::LynxError(base::LynxError(
            error::E_BTS_RUNTIME_ERROR,
            "call sendGlobalEvent on invalid state, will be ignored",
            base::LynxErrorLevel::Error)));
      } else {
        task();
      }
    } else {
      runtime->call(std::move((task)));
    }
  });
}

void JSProxyAndroid::CallJSIntersectionObserver(int32_t observer_id,
                                                int32_t callback_id,
                                                long args_id) {
  actor_->Act([observer_id, callback_id, args_id,
               jni_object = jni_object_](auto& runtime) mutable {
    auto js_runtime = runtime->GetJSRuntime();
    if (js_runtime == nullptr) {
      LOGR(
          "try CallJSIntersectionObserver before js context is ready! "
          "observer_id:"
          << observer_id << " callback_id:" << callback_id
          << " args_id:" << args_id << &runtime);
      return;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    base::android::ScopedLocalJavaRef<jobject> local_ref(*jni_object);
    if (local_ref.IsNull()) {
      return;
    }
    auto args = Java_JSProxy_getArgs(env, local_ref.Get(), args_id);
    if (args.IsNull()) {
      return;
    }
    piper::Scope scope(*js_runtime);
    auto data = jsObjectFromJavaOnlyMap(AttachCurrentThread(), args.Get(),
                                        js_runtime.get());
    if (!data) {
      js_runtime->reportJSIException(
          BUILD_JSI_NATIVE_EXCEPTION("CallJSIntersectionObserver fail! Reason: "
                                     "Transfer java value to js value fail."));
      return;
    }
    runtime->CallIntersectionObserver(observer_id, callback_id,
                                      std::move(*data));
  });
}

void JSProxyAndroid::CallJSApiCallbackWithValue(int32_t callback_id,
                                                long args_id) {
  actor_->Act([callback_id, args_id,
               jni_object = jni_object_](auto& runtime) mutable {
    auto js_runtime = runtime->GetJSRuntime();
    if (js_runtime == nullptr) {
      LOGR(
          "try CallJSApiCallbackWithValue before js context is ready! "
          "callback_id:"
          << callback_id << " args_id:" << args_id << &runtime);
      return;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    base::android::ScopedLocalJavaRef<jobject> local_ref(*jni_object);
    if (local_ref.IsNull()) {
      return;
    }
    auto args = Java_JSProxy_getArgs(env, local_ref.Get(), args_id);
    if (args.IsNull()) {
      return;
    }
    piper::Scope scope(*js_runtime);
    auto data = jsObjectFromJavaOnlyMap(AttachCurrentThread(), args.Get(),
                                        js_runtime.get());
    if (!data) {
      js_runtime->reportJSIException(
          BUILD_JSI_NATIVE_EXCEPTION("CallJSApiCallbackWithValue fail! Reason: "
                                     "Transfer java value to js value fail."));
      return;
    }
    runtime->CallJSApiCallbackWithValue(piper::ApiCallBack(callback_id),
                                        piper::Value(*data));
  });
}

void JSProxyAndroid::EvaluateScript(const std::string& url, std::string script,
                                    int32_t callback_id) {
  actor_->Act(
      [url, script = std::move(script), callback_id](auto& runtime) mutable {
        runtime->EvaluateScript(url, std::move(script),
                                piper::ApiCallBack(callback_id));
      });
}

void JSProxyAndroid::RejectDynamicComponentLoad(const std::string& url,
                                                int32_t callback_id,
                                                int32_t err_code,
                                                const std::string& err_msg) {
  actor_->Act([url, callback_id, err_code, err_msg](auto& runtime) {
    runtime->CallJSApiCallbackWithValue(
        piper::ApiCallBack(callback_id),
        tasm::lazy_bundle::ConstructErrorMessageForBTS(url, err_code, err_msg));
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
