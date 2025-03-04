// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/base/android/jni_helper.h"
#include "core/base/thread/atomic_lifecycle.h"
#include "core/build/gen/LynxTemplateRender_jni.h"
#include "core/public/lynx_extension_delegate.h"
#include "core/public/ui_delegate.h"
#include "core/renderer/data/android/platform_data_android.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "core/renderer/utils/android/event_converter_android.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/resource/lynx_resource_loader_android.h"
#include "core/runtime/bindings/jsi/modules/android/module_factory_android.h"
#include "core/services/timing_handler/android/timing_collector_android.h"
#include "core/shell/android/lynx_runtime_wrapper_android.h"
#include "core/shell/android/lynx_template_renderer_android.h"
#include "core/shell/android/native_facade_android.h"
#include "core/shell/android/native_facade_reporter_android.h"
#include "core/shell/android/platform_call_back_android.h"
#include "core/shell/android/tasm_platform_invoker_android.h"
#include "core/shell/lynx_engine_proxy_impl.h"
#include "core/shell/lynx_runtime_proxy_impl.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/module_delegate_impl.h"

using lynx::base::AtomicLifecycle;
using lynx::base::android::AttachCurrentThread;
using lynx::base::android::JavaOnlyArray;
using lynx::base::android::JavaOnlyMap;
using lynx::base::android::JNIConvertHelper;
using lynx::base::android::ScopedGlobalJavaRef;
using lynx::lepus::Value;
using lynx::shell::LynxShell;

namespace {

Value ConvertJavaData(JNIEnv* env, jobject j_data, jint length) {
  if (j_data == nullptr || length <= 0) {
    return Value();
  }

  char* data = static_cast<char*>(env->GetDirectBufferAddress(j_data));
  if (data == nullptr) {
    return Value();
  }

  lynx::tasm::LepusDecoder decoder;
  return decoder.DecodeMessage(data, length);
}

// TODO(heshan):use template optimize java only container
template <typename T>
std::unique_ptr<JavaOnlyArray> ConvertToJavaArray(const std::vector<T>& vec) {
  auto array = std::make_unique<JavaOnlyArray>();
  for (auto index : vec) {
    array->PushInt(static_cast<int32_t>(index));
  }
  return array;
}

std::unique_ptr<JavaOnlyArray> ConvertToJavaArray(
    const std::vector<std::string>& vec) {
  auto array = std::make_unique<JavaOnlyArray>();
  for (const auto& str : vec) {
    array->PushString(str);
  }
  return array;
}

template <typename T>
std::unique_ptr<JavaOnlyArray> ConvertToJavaArray(
    const lynx::base::Vector<T>& vec) {
  auto array = std::make_unique<JavaOnlyArray>();
  for (auto index : vec) {
    array->PushInt(static_cast<int32_t>(index));
  }
  return array;
}

std::unique_ptr<JavaOnlyArray> ConvertToJavaArray(
    const lynx::base::Vector<std::string>& vec) {
  auto array = std::make_unique<JavaOnlyArray>();
  for (const auto& str : vec) {
    array->PushString(str);
  }
  return array;
}

template <typename T>
void AddToJavaMap(JavaOnlyMap& map, const std::string& key, const T& vec) {
  map.PushArray(key, ConvertToJavaArray(vec).get());
}

void InternalLoadTemplate(JNIEnv* env, jlong ptr, jlong lifecycle,
                          jstring j_url, jbyteArray j_binary,
                          const Value& value, const bool read_only_value,
                          bool enable_pre_painting,
                          const std::string& processor_name,
                          jobject android_template_data,
                          bool enable_recycle_template_bundle) {
  // TODO(songshourui.null): add a method to get template_data with
  // android_template_data
  std::shared_ptr<lynx::tasm::TemplateData> template_data =
      value.IsNil() ? nullptr
                    : std::make_shared<lynx::tasm::TemplateData>(
                          value, read_only_value, processor_name);
  if (template_data) {
    template_data->SetPlatformData(
        std::make_unique<lynx::tasm::PlatformDataAndroid>(
            lynx::base::android::ScopedGlobalJavaRef<jobject>(
                env, android_template_data)));
  }

  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->LoadTemplate(
      JNIConvertHelper::ConvertToString(env, j_url),
      JNIConvertHelper::ConvertJavaBinary(env, j_binary), template_data,
      enable_pre_painting, enable_recycle_template_bundle);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void InternalLoadSSRData(JNIEnv* env, jlong ptr, jlong lifecycle,
                         jbyteArray j_binary, const Value& value,
                         const bool read_only_value,
                         const std::string& processor_name,
                         jobject android_template_data) {
  // TODO(songshourui.null): add a method to get template_data with
  // android_template_data
  std::shared_ptr<lynx::tasm::TemplateData> template_data =
      value.IsNil() ? nullptr
                    : std::make_shared<lynx::tasm::TemplateData>(
                          value, read_only_value, processor_name);
  if (template_data) {
    template_data->SetPlatformData(
        std::make_unique<lynx::tasm::PlatformDataAndroid>(
            lynx::base::android::ScopedGlobalJavaRef<jobject>(
                env, android_template_data)));
  }

  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->LoadSSRData(
      JNIConvertHelper::ConvertJavaBinary(env, j_binary), template_data);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

std::shared_ptr<lynx::tasm::TemplateData> ConvertToTemplateData(
    JNIEnv* env, jlong ptr, jboolean readOnly, jstring processorName,
    jobject android_template_data) {
  // TODO(songshourui.null): add a method to get template_data with
  // android_template_data
  auto template_data = std::make_shared<lynx::tasm::TemplateData>(
      ptr ? *(reinterpret_cast<Value*>(ptr)) : Value(), readOnly,
      JNIConvertHelper::ConvertToString(env, processorName));
  if (template_data) {
    template_data->SetPlatformData(
        std::make_unique<lynx::tasm::PlatformDataAndroid>(
            lynx::base::android::ScopedGlobalJavaRef<jobject>(
                env, android_template_data)));
  }
  return template_data;
}
}  // namespace

jlong Create(JNIEnv* env, jclass jcaller, jlong timing_collector_android,
             jlong runtime_wrapper_ptr, jobject native_facade,
             jobject native_facade_reporter, jobject platform_loader,
             jint thread_strategy, jboolean enable_layout_safe_point,
             jboolean enable_layout_only, jint screen_width, jint screen_height,
             jfloat density, jstring locale, jboolean enable_js,
             jboolean enable_multi_async_thread,
             jboolean enable_pre_update_data, jboolean enable_auto_concurrency,
             jboolean enable_vsync_aligned_msg_loop,
             jboolean enable_async_hydration, jboolean enable_js_group_thread,
             jstring js_group_thread_name, jobject tasm_platform_invoker,
             jlong white_board_ptr, jlong ui_delegate_ptr,
             jboolean use_invoke_ui_method) {
  auto* ui_delegate =
      reinterpret_cast<lynx::tasm::UIDelegate*>(ui_delegate_ptr);

  auto lynx_env_config =
      ui_delegate->UsesLogicalPixels()
          ? lynx::tasm::LynxEnvConfig(screen_width / density,
                                      screen_height / density, 1.f, density)
          : lynx::tasm::LynxEnvConfig(screen_width, screen_height, density,
                                      1.f);
  auto* runtime_wrapper =
      reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid*>(
          runtime_wrapper_ptr);
  // TODO(chenyouhui): Create LazyBundleLoader in LynxShellBuilder
  auto loader = std::make_shared<lynx::tasm::LazyBundleLoader>(
      std::make_shared<lynx::shell::LynxResourceLoaderAndroid>(
          env, platform_loader));
  lynx::shell::ShellOption shell_option;
  shell_option.enable_multi_tasm_thread_ = enable_multi_async_thread;
  shell_option.enable_multi_layout_thread_ = enable_multi_async_thread;
  shell_option.enable_js_ = enable_js;
  shell_option.enable_auto_concurrency_ = enable_auto_concurrency;
  shell_option.enable_vsync_aligned_msg_loop_ = enable_vsync_aligned_msg_loop;
  shell_option.enable_async_hydration_ = enable_async_hydration;
  shell_option.enable_js_group_thread_ = enable_js_group_thread;
  shell_option.js_group_thread_name_ =
      JNIConvertHelper::ConvertToString(env, js_group_thread_name);
  shell_option.instance_id_ =
      runtime_wrapper == nullptr
          ? -1  // {kUnknownInstanceId};
          : runtime_wrapper->GetRuntimeActor()->GetInstanceId();
  std::shared_ptr<lynx::tasm::WhiteBoard> white_board = nullptr;
  if (white_board_ptr != 0) {
    white_board = *reinterpret_cast<std::shared_ptr<lynx::tasm::WhiteBoard>*>(
        white_board_ptr);
  }
  std::shared_ptr<lynx::tasm::timing::TimingCollectorAndroid>
      sp_timing_collector_android = nullptr;
  if (timing_collector_android != 0) {
    sp_timing_collector_android = *reinterpret_cast<
        std::shared_ptr<lynx::tasm::timing::TimingCollectorAndroid>*>(
        timing_collector_android);
  }

  return reinterpret_cast<jlong>(
      lynx::shell::LynxShellBuilder()
          .SetUseInvokeUIMethodFunction(use_invoke_ui_method)
          .SetNativeFacade(std::make_unique<lynx::shell::NativeFacadeAndroid>(
              env, native_facade))
          .SetNativeFacadeReporter(
              std::make_unique<lynx::shell::NativeFacadeReporterAndroid>(
                  env, native_facade_reporter))
          .SetPaintingContextPlatformImpl(ui_delegate->CreatePaintingContext())
          .SetLynxEnvConfig(lynx_env_config)
          .SetEnableElementManagerVsyncMonitor(true)
          .SetEnableLayoutOnly(enable_layout_only)
          .SetWhiteBoard(white_board)
          .SetLazyBundleLoader(loader)
          .SetTasmLocale(JNIConvertHelper::ConvertToString(env, locale))
          .SetEnablePreUpdateData(enable_pre_update_data)
          .SetLayoutContextPlatformImpl(ui_delegate->CreateLayoutContext())
          .SetStrategy(static_cast<lynx::base::ThreadStrategyForRendering>(
              thread_strategy))
          .SetEngineActor(
              [loader](auto& actor) { loader->SetEngineActor(actor); })
          .SetRuntimeActor((runtime_wrapper != nullptr)
                               ? runtime_wrapper->GetRuntimeActor()
                               : nullptr)
          .SetTimingActor((runtime_wrapper != nullptr)
                              ? runtime_wrapper->GetTimingActor()
                              : nullptr)
          .SetShellOption(shell_option)
          .SetPropBundleCreator(ui_delegate->CreatePropBundleCreator())
          .SetTasmPlatformInvoker(
              std::make_unique<lynx::shell::TasmPlatformInvokerAndroid>(
                  env, tasm_platform_invoker))
          .SetTimingCollectorPlatform(sp_timing_collector_android)
          .build());
}

void Destroy(JNIEnv* env, jclass jcaller, jlong ptr) {
  delete reinterpret_cast<LynxShell*>(ptr);
}

jlong LifecycleCreate(JNIEnv* env, jclass jclass) {
  return reinterpret_cast<jlong>(new AtomicLifecycle());
}

jboolean LifecycleTryTerminate(JNIEnv* env, jclass jclass, jlong lifecycle) {
  return AtomicLifecycle::TryTerminate(
      reinterpret_cast<AtomicLifecycle*>(lifecycle));
}

void LifecycleDestroy(JNIEnv* env, jclass jclass, jlong lifecycle) {
  delete reinterpret_cast<AtomicLifecycle*>(lifecycle);
}

jint GetInstanceId(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle) {
  jint id = -1;
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return id;
  }
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  id = shell->GetInstanceId();
  AtomicLifecycle::TryFree(lifecycle_ptr);
  return id;
}

void AttachRuntime(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                   jlong background_runtime) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto* runtime_wrapper =
      reinterpret_cast<lynx::shell::LynxRuntimeWrapperAndroid*>(
          background_runtime);
  reinterpret_cast<LynxShell*>(ptr)->AttachRuntime(
      runtime_wrapper->GetModuleManager());
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void InitRuntime(JNIEnv* env, jclass jcaller, jlong ptr,
                 jobject resource_loader, jobject java_module_factory,
                 jstring java_group_id, jobjectArray preload_js_paths,
                 jboolean force_reload_js_core,
                 jboolean force_use_light_weight_js_engine,
                 jboolean enable_pending_js_task, jboolean enable_user_bytecode,
                 jstring bytecode_source_url, jlong ui_delegate_ptr) {
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  auto module_manager = std::make_shared<lynx::piper::LynxModuleManager>();
  module_manager->SetPlatformModuleFactory(
      std::make_unique<lynx::piper::ModuleFactoryAndroid>(env,
                                                          java_module_factory));
  std::string group_id = JNIConvertHelper::ConvertToString(env, java_group_id);
  std::string source_url =
      JNIConvertHelper::ConvertToString(env, bytecode_source_url);
  auto paths = JNIConvertHelper::ConvertJavaStringArrayToStringVector(
      env, preload_js_paths);

  auto loader = std::make_shared<lynx::shell::LynxResourceLoaderAndroid>(
      env, resource_loader);
  auto on_runtime_actor_created = [&module_manager, &shell,
                                   ui_delegate_ptr](auto& actor) {
    module_manager->initBindingPtr(
        module_manager,
        std::make_shared<lynx::shell::ModuleDelegateImpl>(actor));
    if (ui_delegate_ptr != 0) {
      auto ui_delegate =
          reinterpret_cast<lynx::tasm::UIDelegate*>(ui_delegate_ptr);
      module_manager->SetModuleFactory(ui_delegate->GetCustomModuleFactory());
      auto runtime_proxy = std::make_shared<lynx::shell::LynxRuntimeProxyImpl>(
          shell->GetRuntimeActor());
      auto engine_proxy = std::make_shared<lynx::shell::LynxEngineProxyImpl>(
          shell->GetEngineActor());
      ui_delegate->OnLynxCreate(std::move(engine_proxy),
                                std::move(runtime_proxy), nullptr, nullptr,
                                nullptr);
    }
  };
  shell->InitRuntime(group_id, loader, module_manager,
                     std::move(on_runtime_actor_created), std::move(paths),
                     force_reload_js_core, force_use_light_weight_js_engine,
                     enable_pending_js_task, enable_user_bytecode, source_url);
}

void StartRuntime(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->StartJsRuntime();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void ProcessRender(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->ForceFlush();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SetEnableUIFlush(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                      jboolean enable_ui_flush) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->SetEnableUIFlush(enable_ui_flush);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void OnEnterForeground(JNIEnv* env, jclass jcaller, jlong ptr,
                       jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->OnEnterForeground();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void OnEnterBackground(JNIEnv* env, jclass jcaller, jlong ptr,
                       jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->OnEnterBackground();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void LoadSSRDataByPreParsedData(JNIEnv* env, jclass jcaller, jlong ptr,
                                jlong lifecycle, jbyteArray ssr_data,
                                jlong data, jboolean read_only, jstring name,
                                jobject template_data) {
  std::string processor_name = JNIConvertHelper::ConvertToString(env, name);
  InternalLoadSSRData(env, ptr, lifecycle, ssr_data,
                      data ? *(reinterpret_cast<Value*>(data)) : Value(),
                      read_only, processor_name, template_data);
}

void LoadTemplateByPreParsedData(JNIEnv* env, jclass jcaller, jlong ptr,
                                 jlong lifecycle, jstring j_url,
                                 jbyteArray j_binary, jint is_pre_painting,
                                 jboolean enable_recycle_template_bundle,
                                 jlong data, jboolean readOnly, jstring name,
                                 jobject template_data) {
  std::string processor_name = JNIConvertHelper::ConvertToString(env, name);
  InternalLoadTemplate(env, ptr, lifecycle, j_url, j_binary,
                       data ? *(reinterpret_cast<Value*>(data)) : Value(),
                       readOnly, is_pre_painting == 1, processor_name,
                       template_data, enable_recycle_template_bundle);
}

void LoadTemplateBundleByPreParsedData(JNIEnv* env, jclass jcaller, jlong ptr,
                                       jlong lifecycle, jstring j_url,
                                       jlong bundlePtr, jint is_pre_painting,
                                       jlong data, jboolean readOnly,
                                       jstring processorName,
                                       jobject android_template_data,
                                       jboolean enable_dump_element) {
  // TODO(songshourui.null): add a method to get template_data with
  // android_template_data
  std::string processor_name =
      JNIConvertHelper::ConvertToString(env, processorName);
  std::string url = JNIConvertHelper::ConvertToString(env, j_url);
  auto value = data ? *(reinterpret_cast<Value*>(data)) : Value();
  auto template_data = value.IsNil()
                           ? nullptr
                           : std::make_shared<lynx::tasm::TemplateData>(
                                 value, readOnly, processor_name);
  if (template_data) {
    template_data->SetPlatformData(
        std::make_unique<lynx::tasm::PlatformDataAndroid>(
            lynx::base::android::ScopedGlobalJavaRef<jobject>(
                env, android_template_data)));
  }

  auto bundle = reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(bundlePtr);
  lynx::tasm::LynxTemplateBundle copied_bundle = *bundle;

  if (bundle->GetContainsElementTree()) {
    // The Element Bundle cannot be reused for rendering and is disposable.
    // Therefore, a deep copy is needed here to ensure that the internal
    // rendering does not invalidate the element bundle passed in by the
    // business.
    copied_bundle.SetElementBundle(bundle->GetElementBundle().DeepClone());
  }

  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->LoadTemplateBundle(
      url, std::move(copied_bundle), template_data, is_pre_painting == 1,
      enable_dump_element);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void PreloadLazyBundles(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                        jobjectArray urls) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->PreloadLazyBundles(
      JNIConvertHelper::ConvertJavaStringArrayToStringVector(env, urls));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

jboolean RegisterLazyBundle(JNIEnv* env, jclass jcaller, jlong ptr,
                            jlong lifecycle, jstring url, jlong bundle_ptr) {
  const auto& bundle =
      *reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(bundle_ptr);
  // only valid bundles from dynamic component templates will be passed to the
  // shell
  if (bundle.IsCard()) {
    return false;
  }
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return false;
  }
  reinterpret_cast<LynxShell*>(ptr)->RegisterLazyBundle(
      JNIConvertHelper::ConvertToString(env, url), bundle);
  AtomicLifecycle::TryFree(lifecycle_ptr);
  return true;
}

void UpdateDataByPreParsedData(JNIEnv* env, jclass jcaller, jlong ptr,
                               jlong lifecycle, jlong data,
                               jstring j_processor_name,
                               jboolean read_only_value,
                               jobject android_template_data) {
  // TODO(songshourui.null): add a method to get template_data with
  // android_template_data
  const auto& value = data ? *(reinterpret_cast<Value*>(data)) : Value();
  std::string processor_name =
      JNIConvertHelper::ConvertToString(env, j_processor_name);
  std::shared_ptr<lynx::tasm::TemplateData> template_data =
      std::make_shared<lynx::tasm::TemplateData>(value, read_only_value,
                                                 processor_name);
  if (template_data) {
    template_data->SetPlatformData(
        std::make_unique<lynx::tasm::PlatformDataAndroid>(
            lynx::base::android::ScopedGlobalJavaRef<jobject>(
                env, android_template_data)));
  }

  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->UpdateDataByParsedData(template_data);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void UpdateMetaData(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                    jlong dataPtr, jstring processorName, jboolean readOnly,
                    jobject templateData, jlong globalPropsPtr) {
  std::shared_ptr<lynx::tasm::TemplateData> updated_data = nullptr;
  if (dataPtr != 0) {
    std::string processor_name =
        JNIConvertHelper::ConvertToString(env, processorName);
    const auto& value = *(reinterpret_cast<Value*>(dataPtr));
    updated_data = std::make_shared<lynx::tasm::TemplateData>(value, readOnly,
                                                              processor_name);
    updated_data->SetPlatformData(
        std::make_unique<lynx::tasm::PlatformDataAndroid>(
            lynx::base::android::ScopedGlobalJavaRef<jobject>(env,
                                                              templateData)));
  }

  auto updated_global_props =
      globalPropsPtr ? *(reinterpret_cast<Value*>(globalPropsPtr)) : Value();

  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<lynx::base::AtomicLifecycle*>(lifecycle);
  if (!lynx::base::AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->UpdateMetaData(updated_data,
                                                    updated_global_props);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void ResetDataByPreParsedData(JNIEnv* env, jclass jcaller, jlong ptr,
                              jlong lifecycle, jlong data_ptr,
                              jstring j_processor_name, jboolean read_only,
                              jobject android_template_data) {
  // TODO(songshourui.null): add a method to get template_data with
  // android_template_data
  const auto& value =
      data_ptr ? *(reinterpret_cast<Value*>(data_ptr)) : Value();
  std::string processor_name =
      JNIConvertHelper::ConvertToString(env, j_processor_name);

  std::shared_ptr<lynx::tasm::TemplateData> template_data =
      std::make_shared<lynx::tasm::TemplateData>(value, read_only,
                                                 processor_name);
  if (template_data) {
    template_data->SetPlatformData(
        std::make_unique<lynx::tasm::PlatformDataAndroid>(
            lynx::base::android::ScopedGlobalJavaRef<jobject>(
                env, android_template_data)));
  }

  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->ResetDataByParsedData(template_data);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void ReloadTemplate(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                    jlong j_data_ptr, jlong j_props_ptr,
                    jstring j_data_processor_name, jboolean j_data_readonly,
                    jobject j_global_props, jobject android_template_data) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  /*
   * Null j global props -> Nil Value;
   * Empty j global props -> Table Value;
   */
  if (j_global_props == nullptr) {
    reinterpret_cast<LynxShell*>(ptr)->ReloadTemplate(
        ConvertToTemplateData(env, j_data_ptr, j_data_readonly,
                              j_data_processor_name, android_template_data));
  } else {
    Value* prop_value = reinterpret_cast<Value*>(j_props_ptr);
    reinterpret_cast<LynxShell*>(ptr)->ReloadTemplate(
        ConvertToTemplateData(env, j_data_ptr, j_data_readonly,
                              j_data_processor_name, android_template_data),
        prop_value ? *prop_value : Value(lynx::lepus::Dictionary::Create()));
  }
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void UpdateConfig(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                  jobject j_config, jint length) {
  Value config = ConvertJavaData(env, j_config, length);
  if (!config.IsNil()) {
    AtomicLifecycle* lifecycle_ptr =
        reinterpret_cast<AtomicLifecycle*>(lifecycle);
    if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
      return;
    }
    reinterpret_cast<LynxShell*>(ptr)->UpdateConfig(config);
    AtomicLifecycle::TryFree(lifecycle_ptr);
  }
}

void UpdateGlobalProps(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                       jlong j_props) {
  auto* props = reinterpret_cast<Value*>(j_props);
  if (props != nullptr && !props->IsNil()) {
    AtomicLifecycle* lifecycle_ptr =
        reinterpret_cast<AtomicLifecycle*>(lifecycle);
    if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
      return;
    }
    reinterpret_cast<LynxShell*>(ptr)->UpdateGlobalProps(*props);
    AtomicLifecycle::TryFree(lifecycle_ptr);
  }
}

void UpdateViewport(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                    jint width, jint width_mode, jint height, jint height_mode,
                    jfloat scale, jlong ui_delegate_ptr) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto* ui_delegate =
      reinterpret_cast<lynx::tasm::UIDelegate*>(ui_delegate_ptr);
  if (ui_delegate->UsesLogicalPixels()) {
    reinterpret_cast<LynxShell*>(ptr)->UpdateViewport(
        width / scale, width_mode, height / scale, height_mode);
  } else {
    reinterpret_cast<LynxShell*>(ptr)->UpdateViewport(width, width_mode, height,
                                                      height_mode);
  }
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SetSessionStorageItem(JNIEnv* env, jobject jcaller, jlong ptr,
                           jlong lifecycle, jstring key, jlong value,
                           jboolean readonly) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  std::string shared_key =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
  const auto& shared_data =
      value ? *(reinterpret_cast<Value*>(value)) : Value();
  reinterpret_cast<lynx::shell::LynxShell*>(ptr)->SetSessionStorageItem(
      std::move(shared_key),
      std::make_shared<lynx::tasm::TemplateData>(shared_data, readonly));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void GetSessionStorageItem(JNIEnv* env, jobject jcaller, jlong ptr,
                           jlong lifecycle, jstring key, jobject callback) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto shared_key =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
  auto platform_callback =
      std::make_unique<lynx::shell::PlatformCallBackAndroid>(env, callback);
  reinterpret_cast<lynx::shell::LynxShell*>(ptr)->GetSessionStorageItem(
      std::move(shared_key), std::move(platform_callback));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

jdouble SubscribeSessionStorage(JNIEnv* env, jobject jcaller, jlong ptr,
                                jlong lifecycle, jstring key,
                                jobject callBack) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return -1;
  }
  std::string shared_key =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
  auto platform_callback =
      std::make_unique<lynx::shell::PlatformCallBackAndroid>(env, callBack);
  auto id =
      reinterpret_cast<lynx::shell::LynxShell*>(ptr)->SubscribeSessionStorage(
          std::move(shared_key), std::move(platform_callback));
  AtomicLifecycle::TryFree(lifecycle_ptr);
  return id;
}

void UnsubscribeSessionStorage(JNIEnv* env, jobject jcaller, jlong ptr,
                               jlong lifecycle, jstring key, jdouble id) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  std::string shared_key =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
  reinterpret_cast<lynx::shell::LynxShell*>(ptr)->UnSubscribeSessionStorage(
      std::move(shared_key), id);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void UpdateScreenMetrics(JNIEnv* env, jclass jcaller, jlong ptr,
                         jlong lifecycle, jint width, jint height, jfloat scale,
                         jlong ui_delegate_ptr) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto* ui_delegate =
      reinterpret_cast<lynx::tasm::UIDelegate*>(ui_delegate_ptr);
  if (ui_delegate->UsesLogicalPixels()) {
    reinterpret_cast<LynxShell*>(ptr)->UpdateScreenMetrics(
        width / scale, height / scale, scale);
  } else {
    reinterpret_cast<LynxShell*>(ptr)->UpdateScreenMetrics(width, height,
                                                           scale);
  }
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SetFontScale(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                  jfloat scale) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->SetFontScale(scale);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SetPlatformConfig(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                       jstring platform_config) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->SetPlatformConfig(
      JNIConvertHelper::ConvertToString(env, platform_config));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void UpdateFontScale(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                     jfloat scale) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->UpdateFontScale(scale);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SyncFetchLayoutResult(JNIEnv* env, jclass jcaller, jlong ptr,
                           jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->SyncFetchLayoutResult();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SendTouchEvent(JNIEnv* env, jclass jcaller, jlong ptr, jstring name,
                    jint tag, jfloat client_x, jfloat client_y, jfloat page_x,
                    jfloat page_y, jfloat view_x, jfloat view_y) {
  reinterpret_cast<LynxShell*>(ptr)->SendTouchEvent(
      JNIConvertHelper::ConvertToString(env, name), tag, view_x, view_y,
      client_x, client_y, page_x, page_y);
}

void SendCustomEvent(JNIEnv* env, jclass jcaller, jlong ptr, jstring name,
                     jint tag, jobject params, jint length,
                     jstring params_name) {
  reinterpret_cast<LynxShell*>(ptr)->SendCustomEvent(
      JNIConvertHelper::ConvertToString(env, name), tag,
      ConvertJavaData(env, params, length),
      JNIConvertHelper::ConvertToString(env, params_name));
}

void SendGestureEvent(JNIEnv* env, jobject jcaller, jlong ptr, jstring name,
                      jint tag, jint gesture_id, jobject params, jint length) {
  // Convert the Java objects and invoke the SendGestureEvent method.
  reinterpret_cast<LynxShell*>(ptr)->SendGestureEvent(
      tag, gesture_id, JNIConvertHelper::ConvertToString(env, name),
      ConvertJavaData(env, params, length));
}

void SendSsrGlobalEvent(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                        jstring name, jobject buffer, jint length) {
  std::string event_name = JNIConvertHelper::ConvertToString(env, name);
  Value params = ConvertJavaData(env, buffer, length);
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->SendSsrGlobalEvent(event_name, params);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SendGlobalEventToLepus(JNIEnv* env, jclass jcaller, jlong ptr,
                            jlong lifecycle, jstring name, jobject buffer,
                            jint length) {
  std::string event_name = JNIConvertHelper::ConvertToString(env, name);
  Value params = ConvertJavaData(env, buffer, length);
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->SendGlobalEventToLepus(event_name, params);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void TriggerEventBus(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                     jstring name, jobject buffer, jint length) {
  std::string event_name = JNIConvertHelper::ConvertToString(env, name);
  Value params = ConvertJavaData(env, buffer, length);
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->TriggerEventBus(event_name, params);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void OnPseudoStatusChanged(JNIEnv* env, jclass jcaller, jlong ptr, jint tag,
                           jint pre, jint current) {
  reinterpret_cast<LynxShell*>(ptr)->OnPseudoStatusChanged(
      static_cast<int32_t>(tag), static_cast<int32_t>(pre),
      static_cast<int32_t>(current));
}

void GetDataAsync(JNIEnv* env, jobject jcaller, jlong ptr, jlong lifecycle,
                  jint tag) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto data = reinterpret_cast<LynxShell*>(ptr)->GetCurrentData();
  AtomicLifecycle::TryFree(lifecycle_ptr);
  if (data != nullptr) {
    lynx::tasm::LepusEncoder encoder;
    std::vector<int8_t> encoded_data = encoder.EncodeMessage(*data);
    if (!encoded_data.empty()) {
      Java_LynxTemplateRender_getDataBack(
          env, jcaller,
          env->NewDirectByteBuffer(encoded_data.data(), encoded_data.size()),
          tag);
    }
  }
}

jobject GetPageDataByKey(JNIEnv* env, jclass jcaller, jlong ptr,
                         jlong lifecycle, jobjectArray keys) {
  auto keys_vec =
      JNIConvertHelper::ConvertJavaStringArrayToStringVector(env, keys);
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return nullptr;
  }
  auto data =
      reinterpret_cast<LynxShell*>(ptr)->GetPageDataByKey(std::move(keys_vec));
  AtomicLifecycle::TryFree(lifecycle_ptr);

  lynx::tasm::LepusEncoder encoder;
  std::vector<int8_t> encoded_data = encoder.EncodeMessage(data);
  if (!encoded_data.empty()) {
    auto buffer =
        env->NewDirectByteBuffer(encoded_data.data(), encoded_data.size());
    auto result_from_java =
        Java_LynxTemplateRender_decodeByteBuffer(env, buffer);
    return env->NewLocalRef(result_from_java.Get());  // NOLINT
  }
  return nullptr;
}

jobject GetAllJsSource(JNIEnv* env, jclass jcaller, jlong ptr,
                       jlong lifecycle) {
  JavaOnlyMap jni_map;
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return env->NewLocalRef(jni_map.jni_object());  // NOLINT
  }
  for (const auto& item : reinterpret_cast<LynxShell*>(ptr)->GetAllJsSource()) {
    jni_map.PushString(item.first, item.second);
  }
  AtomicLifecycle::TryFree(lifecycle_ptr);
  return env->NewLocalRef(jni_map.jni_object());  // NOLINT
}

void RenderChild(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                 jint tag, jint index, jlong operation_id) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->RenderListChild(
      tag, static_cast<uint32_t>(index), operation_id);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void UpdateChild(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                 jint tag, jint sign, jint index, jlong operation_id) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->UpdateListChild(
      tag, static_cast<uint32_t>(sign), static_cast<uint32_t>(index),
      operation_id);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void RemoveChild(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                 jint tag, jint sign) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->RemoveListChild(
      tag, static_cast<uint32_t>(sign));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

jint ObtainChild(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                 jint tag, jint index, jlong operation_id,
                 jboolean enable_reuse_notification) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return -1;
  }
  jint ret = reinterpret_cast<LynxShell*>(ptr)->ObtainListChild(
      tag, static_cast<uint32_t>(index), operation_id,
      enable_reuse_notification);
  AtomicLifecycle::TryFree(lifecycle_ptr);
  return ret;
}

void ObtainChildAsync(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                      jint tag, jint index, jlong operation_id) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->LoadListNode(static_cast<uint32_t>(tag),
                                                  static_cast<uint32_t>(index),
                                                  operation_id, true);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void RecycleChild(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                  jint tag, jint sign) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->RecycleListChild(
      tag, static_cast<uint32_t>(sign));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void RecycleChildAsync(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                       jint tag, jint sign) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->EnqueueListNode(
      static_cast<uint32_t>(tag), static_cast<uint32_t>(sign));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void ScrollByListContainer(JNIEnv* env, jobject jcaller, jlong ptr,
                           jlong lifecycle, jint sign, jfloat dx, jfloat dy,
                           jfloat originalX, jfloat originalY) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->ScrollByListContainer(
      sign, dx, dy, originalX, originalY);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void ScrollToPosition(JNIEnv* env, jobject jcaller, jlong ptr, jlong lifecycle,
                      jint sign, jint position, jfloat offset, jint align,
                      jboolean smooth) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->ScrollToPosition(sign, position, offset,
                                                      align, smooth);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void ScrollStopped(JNIEnv* env, jobject jcaller, jlong ptr, jlong lifecycle,
                   jint sign) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->ScrollStopped(sign);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

jobject GetListPlatformInfo(JNIEnv* env, jclass jcaller, jlong ptr,
                            jlong lifecycle, jint tag) {
  JavaOnlyMap jni_map;
  auto assembler = [&jni_map](auto* node) {
    jni_map.PushBoolean("diffable", node->Diffable());
    jni_map.PushBoolean("newarch", node->NewArch());
    AddToJavaMap(jni_map, "viewTypes", node->component_info());
    AddToJavaMap(jni_map, "fullspan", node->fullspan());
    AddToJavaMap(jni_map, "stickyTop", node->sticky_top());
    AddToJavaMap(jni_map, "stickyBottom", node->sticky_bottom());
    AddToJavaMap(jni_map, "estimatedHeight", node->estimated_height());
    AddToJavaMap(jni_map, "estimatedHeightPx", node->estimated_height_px());
    AddToJavaMap(jni_map, "itemkeys", node->item_keys());

    JavaOnlyMap diff_result;
    AddToJavaMap(diff_result, "insertions", node->DiffResult().insertions_);
    AddToJavaMap(diff_result, "removals", node->DiffResult().removals_);
    AddToJavaMap(diff_result, "updateFrom", node->DiffResult().update_from_);
    AddToJavaMap(diff_result, "updateTo", node->DiffResult().update_to_);
    AddToJavaMap(diff_result, "moveFrom", node->DiffResult().move_from_);
    AddToJavaMap(diff_result, "moveTo", node->DiffResult().move_to_);
    jni_map.PushMap("diffResult", &diff_result);
  };

  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return nullptr;
  }
  reinterpret_cast<LynxShell*>(ptr)->AssembleListPlatformInfo(
      tag, std::move(assembler));
  AtomicLifecycle::TryFree(lifecycle_ptr);
  return env->NewLocalRef(jni_map.jni_object());  // NOLINT
}

void UpdateI18nResource(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                        jstring key, jstring data, jint) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->UpdateI18nResource(
      JNIConvertHelper::ConvertToString(env, key),
      JNIConvertHelper::ConvertToString(env, data));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SetInitTiming(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                   jlong initStart, jlong initEnd) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void MarkDirty(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->MarkDirty();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void Flush(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  reinterpret_cast<LynxShell*>(ptr)->Flush();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SyncPackageExternalPath(JNIEnv* env, jclass jcaller, jlong ptr,
                             jstring path) {
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm = shell->GetTasm();
  tasm->SyncAndroidPackageExternalPath(
      JNIConvertHelper::ConvertToString(env, path));
}

void SetEnableBytecode(JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle,
                       jboolean enable, jstring url) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->SetEnableBytecode(enable, JNIConvertHelper::ConvertToString(env, url));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void DispatchMessageEvent(JNIEnv* env, jclass jcaller, jlong ptr,
                          jlong lifecycle, jobject event) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->DispatchMessageEvent(lynx::tasm::android::EventConverterAndroid::
                                  ConvertJavaOnlyMapToMessageEvent(env, event));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SetTiming(JNIEnv* env, jobject jcaller, jlong ptr, jlong lifecycle,
               jlong usTimestamp, jstring timingKey, jstring updateFlag) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->SetTiming(usTimestamp,
                   JNIConvertHelper::ConvertToString(env, timingKey),
                   JNIConvertHelper::ConvertToString(env, updateFlag));
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SetSSRTimingData(JNIEnv* env, jobject jcaller, jlong ptr, jlong lifecycle,
                      jstring url, jlong dataSize) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->SetSSRTimingData(JNIConvertHelper::ConvertToString(env, url),
                          dataSize);
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

jobject GetAllTimingInfo(JNIEnv* env, jobject jcaller, jlong ptr,
                         jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    JavaOnlyMap jni_map;
    return env->NewLocalRef(jni_map.jni_object());  // NOLINT
  }
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  auto all_timing =
      lynx::tasm::android::ValueConverterAndroid::ConvertLepusToJavaOnlyMap(
          shell->GetAllTimingInfo());
  AtomicLifecycle::TryFree(lifecycle_ptr);
  return env->NewLocalRef(all_timing.jni_object());  // NOLINT
}

void ClearAllTimingInfo(JNIEnv* env, jobject jcaller, jlong ptr,
                        jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }
  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->ClearAllTimingInfo();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void BindLynxEngineToUIThread(JNIEnv* env, jobject jcaller, jlong ptr,
                              jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }

  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->BindLynxEngineToUIThread();

  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void UnbindLynxEngineFromUIThread(JNIEnv* env, jobject jcaller, jlong ptr,
                                  jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }

  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->UnbindLynxEngineFromUIThread();

  AtomicLifecycle::TryFree(lifecycle_ptr);
}

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

void SetExtensionDelegate(JNIEnv* env, jobject jcaller, jlong ptr,
                          jlong lifecycle, jlong delegate_ptr) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }

  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  auto* extension_delegate =
      reinterpret_cast<lynx::pub::LynxExtensionDelegate*>(delegate_ptr);
  shell->RegisterModuleFactory(extension_delegate->CreateModuleFactory());
  shell->RegisterRuntimeLifecycleObserver(
          extension_delegate->GetRuntimeLifecycleObserver(),
          [](fml::RefPtr<fml::TaskRunner> js_runner) {} /* TODO(chenyouhui)
          remove this param*/);
  extension_delegate->SetRuntimeTaskRunner(
      shell->GetRunners()->GetJSTaskRunner());

  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void SetContextHasAttached(JNIEnv* env, jobject jcaller, jlong ptr,
                           jlong lifecycle) {
  AtomicLifecycle* lifecycle_ptr =
      reinterpret_cast<AtomicLifecycle*>(lifecycle);
  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }

  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->SetContextHasAttached();
  AtomicLifecycle::TryFree(lifecycle_ptr);
}

void EnforceRelayoutOnCurrentThreadWithUpdatedViewport(
    JNIEnv* env, jclass jcaller, jlong ptr, jlong lifecycle, jint width,
    jint width_mode, jint height, jint height_mode) {
  auto* lifecycle_ptr = reinterpret_cast<AtomicLifecycle*>(lifecycle);

  if (!AtomicLifecycle::TryLock(lifecycle_ptr)) {
    return;
  }

  auto* shell = reinterpret_cast<LynxShell*>(ptr);
  shell->LayoutImmediatelyWithUpdatedViewport(
      static_cast<float>(width), static_cast<int32_t>(width_mode),
      static_cast<float>(height), static_cast<int32_t>(height_mode));

  AtomicLifecycle::TryFree(lifecycle_ptr);
}

namespace lynx {
namespace shell {

void LynxTemplateRendererAndroid::RegisterJni(JNIEnv* env) {
  (void)RegisterNativesImpl(env);
}

}  // namespace shell
}  // namespace lynx
