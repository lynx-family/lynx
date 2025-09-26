// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/lynx_runtime_wrapper_android.h"

#include <memory>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/lynx_error_android.h"
#include "core/inspector/observer/inspector_runtime_observer_ng.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/resource/lynx_resource_loader_android.h"
#include "core/runtime/bindings/jsi/modules/android/module_factory_android.h"
#include "core/shell/android/platform_call_back_android.h"
#include "core/shell/common/shell_trace_event_def.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/module_delegate_impl.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxBackgroundRuntime_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxBackgroundRuntime_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxBackgroundRuntime(JNIEnv *env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

using lynx::base::android::AttachCurrentThread;
using lynx::base::android::JNIConvertHelper;

static jlong CreateBackgroundRuntimeWrapper(
    JNIEnv *env, jobject jcaller, jobject java_resource_loader,
    jobject java_module_factory, jlong inspectorObserverPtr,
    jlong white_board_ptr, jstring java_group_id, jstring java_group_name,
    jobjectArray preload_js_paths, jstring bytecode_source_url,
    jint runtime_flags, jlong globalPropsPtr, jboolean debuggable) {
  // create native module manager
  std::shared_ptr<lynx::pub::LynxNativeModuleManager> native_module_manager =
      std::make_shared<lynx::pub::LynxNativeModuleManager>();
  // set platform factory
  native_module_manager->SetPlatformModuleFactory(
      std::make_shared<lynx::piper::ModuleFactoryAndroid>(env,
                                                          java_module_factory));
  std::string group_id = JNIConvertHelper::ConvertToString(env, java_group_id);
  std::string group_name =
      JNIConvertHelper::ConvertToString(env, java_group_name);
  std::string source_url =
      JNIConvertHelper::ConvertToString(env, bytecode_source_url);
  auto paths = JNIConvertHelper::ConvertJavaStringArrayToStringVector(
      env, preload_js_paths);

  auto loader = std::make_shared<lynx::shell::LynxResourceLoaderAndroid>(
      env, java_resource_loader);

  auto on_runtime_actor_created = [&native_module_manager](auto &actor,
                                                           auto &facade_actor) {
    native_module_manager->SetModuleDelegate(
        std::make_shared<lynx::shell::ModuleDelegateImpl>(actor, facade_actor));
  };
  auto native_facade_runtime =
      std::make_unique<lynx::shell::NativeRuntimeFacadeAndroid>(env, jcaller);

  auto bundle_creator =
      std::make_shared<lynx::tasm::PropBundleCreatorAndroid>();

  std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG> *observer = nullptr;

  if (inspectorObserverPtr != 0) {
    observer = reinterpret_cast<
        std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG> *>(
        inspectorObserverPtr);
  }

  std::shared_ptr<lynx::tasm::WhiteBoard> white_board = nullptr;
  if (white_board_ptr != 0) {
    white_board = *reinterpret_cast<std::shared_ptr<lynx::tasm::WhiteBoard> *>(
        white_board_ptr);
  }
  auto *global_props = reinterpret_cast<lynx::lepus::Value *>(globalPropsPtr);

  auto result = lynx::shell::InitRuntimeStandalone(
      group_name, group_id, std::move(native_facade_runtime),
      observer ? *observer : nullptr, loader, native_module_manager,
      bundle_creator, white_board, on_runtime_actor_created, std::move(paths),
      source_url, runtime_flags, global_props);

  // Delete observer to decrease ref count.
  delete observer;

  auto *runtime_wrapper = new lynx::shell::LynxRuntimeWrapperAndroid(
      std::move(result), std::move(group_name));
  return reinterpret_cast<jlong>(runtime_wrapper);
}

void EvaluateTemplateBundle(JNIEnv *env, jobject jcaller, jlong ptr,
                            jstring java_url, jlong template_bundle_ptr,
                            jstring java_js_file) {
  auto *runtime_wrapper =
      reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr);
  auto *bundle =
      reinterpret_cast<lynx::tasm::LynxTemplateBundle *>(template_bundle_ptr);
  if (!bundle || !runtime_wrapper) {
    return;
  }
  auto url = JNIConvertHelper::ConvertToString(env, java_url);
  auto js_file = JNIConvertHelper::ConvertToString(env, java_js_file);
  runtime_wrapper->EvaluateScript(std::move(url), bundle, std::move(js_file));
}

void EvaluateScript(JNIEnv *env, jobject jcaller, jlong ptr, jstring java_url,
                    jbyteArray java_source) {
  auto *runtime_wrapper =
      reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr);
  auto url = JNIConvertHelper::ConvertToString(env, java_url);
  auto script = JNIConvertHelper::ConvertToString(env, java_source);
  runtime_wrapper->EvaluateScript(std::move(url), std::move(script));
}

void SetPresetData(JNIEnv *env, jobject jcaller, jlong nativePtr,
                   jboolean readOnly, jlong presetData) {
  auto *data = reinterpret_cast<lynx::lepus::Value *>(presetData);
  if (data != nullptr && !data->IsNil()) {
    auto *runtime_wrapper =
        reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(nativePtr);
    if (readOnly) {
      // no need clone here
      runtime_wrapper->SetPresetData(*data);
    } else {
      runtime_wrapper->SetPresetData(lynx::lepus::Value::Clone(*data));
    }
  }
}

static void SetSessionStorageItem(JNIEnv *env, jobject jcaller, jlong ptr,
                                  jstring key, jlong value, jboolean readonly) {
  std::string shared_key =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
  const auto &shared_data =
      value ? *(reinterpret_cast<lynx::lepus::Value *>(value))
            : lynx::lepus::Value();
  reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr)
      ->SetSessionStorageItem(
          std::move(shared_key),
          std::make_shared<lynx::tasm::TemplateData>(shared_data, readonly));
}

static void GetSessionStorageItem(JNIEnv *env, jobject jcaller, jlong ptr,
                                  jstring key, jobject callback) {
  auto shared_key =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
  auto platform_callback =
      std::make_unique<lynx::shell::PlatformCallBackAndroid>(env, callback);
  reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr)
      ->GetSessionStorageItem(std::move(shared_key),
                              std::move(platform_callback));
}

static jdouble SubscribeSessionStorage(JNIEnv *env, jobject jcaller, jlong ptr,
                                       jstring key, jobject callBack) {
  std::string shared_key =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
  auto platform_callback =
      std::make_unique<lynx::shell::PlatformCallBackAndroid>(env, callBack);

  return reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr)
      ->SubscribeSessionStorage(std::move(shared_key),
                                std::move(platform_callback));
}

static void UnsubscribeSessionStorage(JNIEnv *env, jobject jcaller, jlong ptr,
                                      jstring key, jdouble id) {
  std::string shared_key =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
  reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr)
      ->UnSubscribeSessionStorage(std::move(shared_key), id);
}

void TransitionToFullRuntime(JNIEnv *env, jobject jcaller, jlong ptr) {
  auto *runtime_wrapper =
      reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr);
  runtime_wrapper->TransitionToFullRuntime();
}

void DestroyWrapper(JNIEnv *env, jclass jcaller, jlong ptr) {
  auto *runtime_wrapper =
      reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr);
  delete runtime_wrapper;
}

void DestroyRuntime(JNIEnv *env, jobject jcaller, jlong ptr) {
  auto *runtime_wrapper =
      reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr);
  runtime_wrapper->DestroyRuntime();
}

jint GetRuntimeId(JNIEnv *env, jobject jcaller, jlong ptr) {
  auto *runtime_wrapper =
      reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid *>(ptr);
  return runtime_wrapper->GetRuntimeId();
}

namespace lynx {
namespace shell {

void LynxRuntimeWrapperAndroid::EvaluateScript(std::string url,
                                               std::string script) {
  uint64_t trace_flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, EVALUATE_SCRIPT_STANDALONE,
              [&url, trace_flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("url", url);
                ctx.event()->add_flow_ids(trace_flow_id);
              });
  runtime_standalone_bundle_.runtime_actor_->Act(
      [url = std::move(url), script = std::move(script),
       trace_flow_id](auto &runtime) mutable {
        runtime->EvaluateScriptStandalone(std::move(url), std::move(script),
                                          trace_flow_id);
      });
}

void LynxRuntimeWrapperAndroid::EvaluateScript(
    std::string url, lynx::tasm::LynxTemplateBundle *bundle,
    std::string js_file) {
  auto js_content = bundle->GetJsBundle().GetJsContent(js_file);
  if (!js_content.has_value()) {
    return;
  }
  auto js_content_val = js_content->get();
  if (js_content_val.IsError()) {
    return;
  }
  auto buffer = js_content_val.GetBuffer();

  if (!buffer || !buffer->data()) {
    return;
  }

  const auto length = buffer->size();
  const auto *data = reinterpret_cast<const char *>(buffer->data());

  uint64_t trace_flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, EVALUATE_SCRIPT_STANDALONE,
              [&url, trace_flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("url", url);
                ctx.event()->add_flow_ids(trace_flow_id);
              });
  runtime_standalone_bundle_.runtime_actor_->Act(
      [url = std::move(url), script = std::string(data, length),
       trace_flow_id](auto &runtime) mutable {
        runtime->EvaluateScriptStandalone(std::move(url), std::move(script),
                                          trace_flow_id);
      });
}

void LynxRuntimeWrapperAndroid::SetPresetData(lepus::Value data) {
  runtime_standalone_bundle_.runtime_actor_->Act(
      [data = std::move(data)](auto &runtime) mutable {
        runtime->OnSetPresetData(std::move(data));
      });
}

void LynxRuntimeWrapperAndroid::SetSessionStorageItem(
    const std::string &key, const std::shared_ptr<tasm::TemplateData> &data) {
  runtime_standalone_bundle_.runtime_actor_->Act(
      [key, data, delegate = runtime_standalone_bundle_.white_board_delegate_](
          auto &runtime) mutable {
        delegate->SetSessionStorageItem(std::move(key), data->GetValue());
      });
}
void LynxRuntimeWrapperAndroid::GetSessionStorageItem(
    const std::string &key, std::unique_ptr<PlatformCallBack> callback) {
  runtime_standalone_bundle_.runtime_actor_->Act(
      [key, callback = std::move(callback),
       facade = runtime_standalone_bundle_.native_runtime_facade_,
       delegate = runtime_standalone_bundle_.white_board_delegate_](
          auto &runtime) mutable {
        auto callback_holder = facade->ActSync(
            [callback = std::move(callback)](auto &facade) mutable {
              return facade->CreatePlatformCallBackHolder(std::move(callback));
            });

        auto value = delegate->GetSessionStorageItem(key);
        delegate->CallPlatformCallbackWithValue(callback_holder, value);
      });
}

double LynxRuntimeWrapperAndroid::SubscribeSessionStorage(
    const std::string &key, std::unique_ptr<PlatformCallBack> callback) {
  auto callback_holder =
      runtime_standalone_bundle_.native_runtime_facade_->ActSync(
          [callback = std::move(callback)](auto &facade) mutable {
            return facade->CreatePlatformCallBackHolder(std::move(callback));
          });

  double callback_id = callback_holder->id();

  runtime_standalone_bundle_.runtime_actor_->Act(
      [key, callback_holder = std::move(callback_holder),
       delegate = runtime_standalone_bundle_.white_board_delegate_](
          auto &runtime) mutable {
        delegate->SubScribeClientSessionStorage(std::move(key),
                                                std::move(callback_holder));
      });
  return callback_id;
}

void LynxRuntimeWrapperAndroid::UnSubscribeSessionStorage(
    const std::string &key, double callback_id) {
  runtime_standalone_bundle_.runtime_actor_->Act(
      [key, callback_id,
       delegate = runtime_standalone_bundle_.white_board_delegate_](
          auto &runtime) mutable {
        delegate->UnsubscribeClientSessionStorage(std::move(key), callback_id);
      });
}

void LynxRuntimeWrapperAndroid::TransitionToFullRuntime() {
  runtime_standalone_bundle_.runtime_actor_->Act(
      [](auto &runtime) { runtime->TransitionToFullRuntime(); });
}

void LynxRuntimeWrapperAndroid::DestroyRuntime() {
  runtime_standalone_bundle_.perf_controller_actor_->Act(
      [instance_id =
           runtime_standalone_bundle_.perf_controller_actor_->GetInstanceId()](
          auto &facade) {
        facade = nullptr;
        lynx::tasm::report::FeatureCounter::Instance()->ClearAndReport(
            instance_id);
      });

  runtime_standalone_bundle_.native_runtime_facade_->Act(
      [instance_id =
           runtime_standalone_bundle_.perf_controller_actor_->GetInstanceId()](
          auto &facade) {
        facade = nullptr;
        lynx::tasm::report::FeatureCounter::Instance()->ClearAndReport(
            instance_id);
      });

  runtime_standalone_bundle_.runtime_actor_->ActAsync(
      [runtime_actor = runtime_standalone_bundle_.runtime_actor_,
       js_group_thread_name = group_name_](auto &runtime) {
        lynx::shell::LynxShell::TriggerDestroyRuntime(runtime_actor,
                                                      js_group_thread_name);
      });
}

void NativeRuntimeFacadeAndroid::ReportError(const base::LynxError &error) {
  JNIEnv *env = AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_ref(jni_object_);
  if (local_ref.IsNull()) {
    return;
  }
  base::android::LynxErrorAndroid error_android(
      error.error_code_, error.error_message_, error.fix_suggestion_,
      error.error_level_, error.custom_info_, error.is_logbox_only_);
  Java_LynxBackgroundRuntime_onErrorOccurred(env, local_ref.Get(),
                                             error_android.jni_object());
}

void NativeRuntimeFacadeAndroid::OnModuleMethodInvoked(
    const std::string &module, const std::string &method, int32_t code) {
  JNIEnv *env = AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_ref(jni_object_);
  if (local_ref.IsNull()) {
    return;
  }
  auto j_module = JNIConvertHelper::ConvertToJNIStringUTF(env, module);
  auto j_method = JNIConvertHelper::ConvertToJNIStringUTF(env, method);
  Java_LynxBackgroundRuntime_onModuleMethodInvoked(
      env, local_ref.Get(), j_module.Get(), j_method.Get(), code);
}

void NativeRuntimeFacadeAndroid::OnEvaluateJavaScriptEnd(
    const std::string &url) {
  JNIEnv *env = AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_ref(jni_object_);
  if (local_ref.IsNull()) {
    return;
  }
  auto j_url = JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  Java_LynxBackgroundRuntime_onEvaluateJavaScriptEnd(env, local_ref.Get(),
                                                     j_url.Get());
}

}  // namespace shell
}  // namespace lynx
