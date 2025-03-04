// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/native_facade_android.h"

#include <utility>

#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "core/base/android/lynx_error_android.h"
#include "core/base/thread/atomic_lifecycle.h"
#include "core/build/gen/NativeFacade_jni.h"
#include "core/renderer/dom/android/lynx_get_ui_result_android.h"
#include "core/renderer/dom/android/lynx_template_bundle_android.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/renderer/utils/android/event_converter_android.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/services/ssr/client/ssr_event_utils.h"
#include "core/shell/lynx_shell.h"

using lynx::base::AtomicLifecycle;
using lynx::base::android::AttachCurrentThread;
using lynx::base::android::GetClass;
using lynx::base::android::INSTANCE_METHOD;
using lynx::base::android::JavaOnlyMap;
using lynx::base::android::JNIConvertHelper;
using lynx::base::android::ScopedLocalJavaRef;
using lynx::lepus::Value;
using lynx::shell::LynxShell;

void AttachEngineToUIThread(JNIEnv* env, jobject jcaller, jlong ptr,
                            jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }

  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->AttachEngineToUIThread();

  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void DetachEngineFromUIThread(JNIEnv* env, jobject jcaller, jlong ptr,
                              jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }

  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->DetachEngineFromUIThread();

  AtomicLifecycle::TryFree(lifecycle_ptr);
}

namespace lynx {
namespace shell {

namespace {

ScopedLocalJavaRef<jobject> ConvertLepusValueToJavaHashMap(JNIEnv* env,
                                                           const Value& value) {
  auto class_hashmap = GetClass(env, "java/util/HashMap");
  auto hashmap_init =
      GetMethod(env, class_hashmap.Get(), INSTANCE_METHOD, "<init>", "()V");
  auto hashmap_put =
      GetMethod(env, class_hashmap.Get(), INSTANCE_METHOD, "put",
                "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

  ScopedLocalJavaRef<jobject> hashmap(
      env, env->NewObject(class_hashmap.Get(), hashmap_init, ""));
  for (const auto& prop : *(value.Table())) {
    if (prop.second.IsString()) {
      auto j_first =
          JNIConvertHelper::ConvertToJNIStringUTF(env, prop.first.str());
      auto j_second =
          JNIConvertHelper::ConvertToJNIStringUTF(env, prop.second.StdString());
      env->CallObjectMethod(hashmap.Get(), hashmap_put, j_first.Get(),
                            j_second.Get());
    }
  }
  return hashmap;
}

jobject ConvertToJavaPerf(const std::unordered_map<int32_t, double>& perf) {
  DCHECK(perf.size() > 0);
  JavaOnlyMap j_perf;
  for (auto& value : perf) {
    j_perf.PushDouble(std::to_string(value.first), value.second);
  }
  return AttachCurrentThread()->NewLocalRef(j_perf.jni_object());  // NOLINT
}

jobject ConvertToJavaPerfTiming(
    const std::unordered_map<int32_t, std::string>& perf_timing) {
  JavaOnlyMap j_perf_timing;
  for (auto& value : perf_timing) {
    j_perf_timing.PushString(std::to_string(value.first), value.second);
  }
  return AttachCurrentThread()->NewLocalRef(  // NOLINT
      j_perf_timing.jni_object());
}
}  // namespace

void NativeFacadeAndroid::RegisterJni(JNIEnv* env) {
  (void)RegisterNativesImpl(env);
}

void NativeFacadeAndroid::OnDataUpdated() {
  Java_NativeFacade_onDataUpdated(AttachCurrentThread(), jni_object_.Get());
}

void NativeFacadeAndroid::OnPageChanged(bool is_first_screen) {
  Java_NativeFacade_onPageChanged(AttachCurrentThread(), jni_object_.Get(),
                                  is_first_screen);
}

void NativeFacadeAndroid::OnTasmFinishByNative() {
  Java_NativeFacade_onTASMFinishedByNative(AttachCurrentThread(),
                                           jni_object_.Get());
}

void NativeFacadeAndroid::OnTemplateLoaded(const std::string& url) {
  Java_NativeFacade_dispatchOnLoaded(AttachCurrentThread(), jni_object_.Get());
}

void NativeFacadeAndroid::OnSSRHydrateFinished(const std::string& url) {
  Java_NativeFacade_onSSRHydrateFinished(AttachCurrentThread(),
                                         jni_object_.Get());
}

void NativeFacadeAndroid::OnRuntimeReady() {
  Java_NativeFacade_onRuntimeReady(AttachCurrentThread(), jni_object_.Get());
}

void NativeFacadeAndroid::ReportError(const base::LynxError& error) {
  JNIEnv* env = AttachCurrentThread();
  base::android::LynxErrorAndroid error_android(
      error.error_code_, error.error_message_, error.fix_suggestion_,
      error.error_level_, error.custom_info_, error.is_logbox_only_);
  Java_NativeFacade_reportError(env, jni_object_.Get(),
                                error_android.jni_object());
}

// issue: #1510
void NativeFacadeAndroid::OnModuleMethodInvoked(const std::string& module,
                                                const std::string& method,
                                                int32_t code) {
  JNIEnv* env = AttachCurrentThread();
  auto j_module = JNIConvertHelper::ConvertToJNIStringUTF(env, module);
  auto j_method = JNIConvertHelper::ConvertToJNIStringUTF(env, method);
  Java_NativeFacade_onModuleFunctionInvoked(
      env, jni_object_.Get(), j_module.Get(), j_method.Get(), code);
}

void NativeFacadeAndroid::OnTimingSetup(const lepus::Value& timing_info) {
  auto j_timing_map = lynx::tasm::android::ValueConverterAndroid::
      ConvertLepusToJavaOnlyMapForTiming(timing_info);
  Java_NativeFacade_onTimingSetup(AttachCurrentThread(), jni_object_.Get(),
                                  j_timing_map.jni_object());
}

void NativeFacadeAndroid::OnTimingUpdate(const lepus::Value& timing_info,
                                         const lepus::Value& update_timing,
                                         const std::string& update_flag) {
  JNIEnv* env = AttachCurrentThread();
  auto j_timing_map = lynx::tasm::android::ValueConverterAndroid::
      ConvertLepusToJavaOnlyMapForTiming(timing_info);
  auto j_update_timing_map = lynx::tasm::android::ValueConverterAndroid::
      ConvertLepusToJavaOnlyMapForTiming(update_timing);
  auto j_update_flag =
      JNIConvertHelper::ConvertToJNIStringUTF(env, update_flag);
  Java_NativeFacade_onTimingUpdate(
      env, jni_object_.Get(), j_timing_map.jni_object(),
      j_update_timing_map.jni_object(), j_update_flag.Get());
}

void NativeFacadeAndroid::OnDynamicComponentPerfReady(
    const lepus::Value& perf_info) {
  auto env = AttachCurrentThread();
  auto perf =
      lynx::tasm::android::ValueConverterAndroid::ConvertLepusToJavaOnlyMap(
          perf_info);
  Java_NativeFacade_onDynamicComponentPerfReady(env, jni_object_.Get(),
                                                perf.jni_object());
}

void NativeFacadeAndroid::OnConfigUpdated(const Value& data) {
  if (data.IsTable() && data.Table()->size() > 0) {
    JNIEnv* env = AttachCurrentThread();
    for (const auto& prop : *(data.Table())) {
      if (!prop.first.IsEqual(tasm::CARD_CONFIG_THEME) ||
          !prop.second.IsTable()) {
        continue;
      }
      auto type =
          JNIConvertHelper::ConvertToJNIStringUTF(env, prop.first.str());
      auto hash_map = ConvertLepusValueToJavaHashMap(env, prop.second);
      Java_NativeFacade_onConfigUpdatedByJS(env, jni_object_.Get(), type.Get(),
                                            hash_map.Get());
    }
  }
}

void NativeFacadeAndroid::OnUpdateDataWithoutChange() {
  Java_NativeFacade_onUpdateDataWithoutChange(AttachCurrentThread(),
                                              jni_object_.Get());
}

void NativeFacadeAndroid::TriggerLepusMethodAsync(
    const std::string& method_name, const lepus::Value& argus) {
  lepus::Value processed_args =
      ssr::FormatEventArgsForAndroid(method_name, argus);
  if (processed_args.IsTable() && processed_args.Table()->size() > 0) {
    JNIEnv* env = base::android::AttachCurrentThread();
    JavaOnlyMap jni_hashmap =
        tasm::android::ValueConverterAndroid::ConvertLepusToJavaOnlyMap(
            processed_args);
    auto java_method =
        JNIConvertHelper::ConvertToJNIStringUTF(env, method_name);
    Java_NativeFacade_triggerLepusBridgeAsync(
        env, jni_object_.Get(), java_method.Get(), jni_hashmap.jni_object());
  }
}

void NativeFacadeAndroid::InvokeUIMethod(
    const tasm::LynxGetUIResult& ui_result, const std::string& method,
    std::unique_ptr<tasm::PropBundle> params, piper::ApiCallBack callback) {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto method_string = JNIConvertHelper::ConvertToJNIStringUTF(env, method);
  lynx::tasm::LynxGetUIResultAndroid result_android(ui_result);
  Java_NativeFacade_InvokeUIMethod(
      env, jni_object_.Get(), result_android.jni_object(), method_string.Get(),
      static_cast<tasm::PropBundleAndroid*>(params.get())
          ->jni_map()
          ->jni_object(),
      callback.id());
}

void NativeFacadeAndroid::FlushJSBTiming(piper::NativeModuleInfo timing) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::FlushJSBTiming",
      [&timing](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("module_name_", timing.module_name_);
        ctx.event()->add_debug_annotations("method_name_", timing.method_name_);
        ctx.event()->add_debug_annotations("method_first_arg_name_",
                                           timing.method_first_arg_name_);
      });
  JNIEnv* env = base::android::AttachCurrentThread();
  JavaOnlyMap map;
  JavaOnlyMap info_map;
  map.PushMap("info", &info_map);
  info_map.PushString("jsb_module_name", timing.module_name_);
  info_map.PushString("jsb_method_name", timing.method_name_);
  info_map.PushString("jsb_name", timing.method_first_arg_name_);
  info_map.PushInt64("jsb_protocol_version", 0);
  info_map.PushString("jsb_bridgesdk", "lynx");
  info_map.PushInt64("jsb_status_code",
                     static_cast<int64_t>(timing.status_code_));
  JavaOnlyMap perf_map;
  map.PushMap("perf", &perf_map);
  perf_map.PushInt64("jsb_call", timing.jsb_call_);
  perf_map.PushInt64("jsb_func_call", timing.jsb_func_call_);
  perf_map.PushInt64("jsb_func_convert_params",
                     timing.jsb_func_convert_params_);
  perf_map.PushInt64("jsb_func_platform_method",
                     timing.jsb_func_platform_method_);
  perf_map.PushInt64("jsb_callback_thread_switch",
                     timing.jsb_callback_thread_switch_);
  perf_map.PushInt64("jsb_callback_thread_switch_waiting",
                     timing.jsb_callback_thread_switch_waiting_);
  perf_map.PushInt64("jsb_callback_call", timing.jsb_callback_call_);
  perf_map.PushInt64("jsb_callback_convert_params",
                     timing.jsb_callback_convert_params_);
  perf_map.PushInt64("jsb_callback_invoke", timing.jsb_callback_invoke_);

  perf_map.PushInt64("jsb_func_call_start", timing.jsb_func_call_start_);
  perf_map.PushInt64("jsb_func_call_end", timing.jsb_func_call_end_);
  perf_map.PushInt64("jsb_callback_thread_switch_start",
                     timing.jsb_callback_thread_switch_start_);
  perf_map.PushInt64("jsb_callback_thread_switch_end",
                     timing.jsb_callback_thread_switch_end_);
  perf_map.PushInt64("jsb_callback_call_start",
                     timing.jsb_callback_call_start_);
  perf_map.PushInt64("jsb_callback_call_end", timing.jsb_callback_call_end_);

  Java_NativeFacade_flushJSBTiming(env, jni_object_.Get(), map.jni_object());
}

void NativeFacadeAndroid::OnTemplateBundleReady(
    tasm::LynxTemplateBundle bundle) {
  JNIEnv* env = AttachCurrentThread();
  auto j_template_bundle =
      tasm::ConstructJTemplateBundleFromNative(std::move(bundle));
  Java_NativeFacade_onTemplateBundleReady(env, jni_object_.Get(),
                                          j_template_bundle.Get());
}

void NativeFacadeAndroid::OnReceiveMessageEvent(runtime::MessageEvent event) {
  if (event.IsSendingToDevTool()) {
    JNIEnv* env = AttachCurrentThread();
    auto event_map =
        tasm::android::EventConverterAndroid::ConvertMessageEventToJavaOnlyMap(
            event);
    Java_NativeFacade_onReceiveMessageEvent(env, jni_object_.Get(),
                                            event_map.jni_object());
  } else {
    // TODO(songshourui.null): impl this after UIContext is supported.
  }
}

}  // namespace shell
}  // namespace lynx
