// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "base/include/fml/platform/android/message_loop_android.h"
#include "base/include/log/logging.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/callstack_util_android.h"
#include "core/base/android/device_utils_android.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/logging_android.h"
#include "core/base/android/lynx_android_blur.h"
#include "core/base/android/lynx_error_android.h"
#include "core/base/android/lynx_white_board_android.h"
#include "core/base/android/piper_data.h"
#include "core/base/android/vsync_monitor_android.h"
#include "core/renderer/css/android/css_color_utils.h"
#include "core/renderer/css/android/css_gradient_utils.h"
#include "core/renderer/dom/android/environment_android.h"
#include "core/renderer/dom/android/lynx_get_ui_result_android.h"
#include "core/renderer/dom/android/lynx_template_bundle_android.h"
#include "core/renderer/dom/android/lynx_view_data_manager_android.h"
#include "core/renderer/tasm/i18n/i18n_binder_android.h"
#include "core/renderer/tasm/react/android/mapbuffer/readable_compact_array_buffer.h"
#include "core/renderer/tasm/react/android/mapbuffer/readable_map_buffer.h"
#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"
#include "core/renderer/ui_wrapper/layout/android/layout_context_android.h"
#include "core/renderer/ui_wrapper/layout/android/layout_node_android.h"
#include "core/renderer/ui_wrapper/layout/android/layout_node_manager_android.h"
#include "core/renderer/ui_wrapper/painting/android/painting_context_android.h"
#include "core/renderer/ui_wrapper/painting/android/ui_delegate_android.h"
#include "core/renderer/utils/android/device_display_info.h"
#include "core/renderer/utils/android/lynx_env_android.h"
#include "core/renderer/utils/android/text_utils_android.h"
#include "core/resource/lynx_resource_loader_android.h"
#include "core/runtime/bindings/jsi/modules/android/callback_impl.h"
#include "core/runtime/bindings/jsi/modules/android/java_attribute_descriptor.h"
#include "core/runtime/bindings/jsi/modules/android/java_method_descriptor.h"
#include "core/runtime/bindings/jsi/modules/android/lynx_module_android.h"
#include "core/runtime/bindings/jsi/modules/android/lynx_promise_impl.h"
#include "core/runtime/bindings/jsi/modules/android/module_factory_android.h"
#include "core/services/event_report/android/event_tracker_android.h"
#include "core/services/feature_count/android/feature_counter_android.h"
#include "core/services/fluency/android/fluency_sample_android.h"
#include "core/services/long_task_timing/android/long_task_monitor_android.h"
#include "core/services/timing_handler/android/timing_collector_android.h"
#include "core/shell/android/js_proxy_android.h"
#include "core/shell/android/lynx_engine_proxy_android.h"
#include "core/shell/android/lynx_runtime_wrapper_android.h"
#include "core/shell/android/lynx_template_renderer_android.h"
#include "core/shell/android/native_facade_android.h"
#include "core/shell/android/native_facade_reporter_android.h"
#include "core/shell/android/platform_call_back_android.h"
#include "core/shell/android/tasm_platform_invoker_android.h"

#if MEMORY_TRACING
#include "core/base/debug/memory_tracer_android.h"
#endif

namespace lynx {

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  lynx::base::android::InitVM(vm);
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::CallStackUtilAndroid::RegisterJNI(env);
  lynx::base::android::DeviceUtilsAndroid::RegisterJNI(env);
  lynx::base::android::JavaOnlyMap::RegisterJni(env);
  lynx::base::android::JavaOnlyArray::RegisterJni(env);
  lynx::base::android::LynxErrorAndroid::RegisterJNI(env);
  lynx::base::android::PiperData::RegisterJni(env);
  lynx::base::android::JReadableMapBuffer::RegisterJni(env);
  lynx::base::android::JReadableCompactArrayBuffer::RegisterJni(env);
  lynx::shell::LynxEngineProxyAndroid::RegisterJNI(env);
  lynx::shell::LynxResourceLoaderAndroid::RegisterJni(env);
  lynx::shell::PlatformCallBackAndroid::RegisterJni(env);
  lynx::shell::TasmPlatformInvokerAndroid::RegisterJni(env);
  lynx::base::LynxWhiteBoardAndroid::RegisterJni(env);
  lynx::tasm::android::DeviceDisplayInfo::RegisterJNI(env);
  lynx::tasm::CSSColorUtils::RegisterJNI(env);
  lynx::tasm::CSSGradientUtils::RegisterJNI(env);
  lynx::tasm::I18nBinderAndroid::RegisterJNI(env);
  lynx::tasm::LynxViewDataManagerAndroid::RegisterJNI(env);
  lynx::tasm::PaintingContextAndroid::RegisterJNI(env);
  lynx::tasm::PaintingContextAndroidRef::RegisterJNI(env);
  lynx::tasm::LayoutContextAndroid::RegisterJNI(env);
  lynx::tasm::LayoutNodeAndroid::RegisterJNI(env);
  lynx::tasm::LayoutNodeManagerAndroid::RegisterJNI(env);
  lynx::tasm::PlatformBundleHolderAndroid::RegisterJNI(env);
  lynx::base::logging::RegisterJNI(env);
  lynx::tasm::LynxGetUIResultAndroid::RegisterJNI(env);
  lynx::tasm::LynxEnvAndroid::RegisterJNI(env);
  lynx::base::LynxAndroidBlur::RegisterJNI(env);
  lynx::tasm::LynxTemplateBundleAndroid::RegisterJNI(env);
  lynx::fml::MessageLoopAndroid::Register(env);
  lynx::tasm::report::RegisterJni(env);
  lynx::tasm::report::RegisterJniFeatureCounter(env);
  lynx::tasm::timing::RegisterJniLongTaskMonitor(env);
  lynx::tasm::timing::TimingCollectorAndroid::RegisterJNI(env);
  lynx::tasm::UIDelegateAndroid::RegisterJNI(env);
  lynx::tasm::TextUtilsAndroidHelper::RegisterJNI(env);
  lynx::piper::LynxModuleAndroid::RegisterJNI(env);
  lynx::piper::JavaMethodDescriptor::RegisterJNI(env);
  lynx::piper::JavaAttributeDescriptor::RegisterJNI(env);
  lynx::piper::ModuleCallbackAndroid::RegisterJNI(env);
  lynx::piper::LynxPromiseImpl::RegisterJNI(env);
  lynx::piper::ModuleFactoryAndroid::RegisterJNIUtils(env);
  lynx::piper::JSBUtilsRegisterJNI(env);
  lynx::piper::JSBUtilsMapRegisterJNI(env);
  lynx::base::android::EnvironmentAndroid::RegisterJNIUtils(env);
  lynx::shell::JSProxyAndroid::RegisterJNIUtils(env);
  lynx::base::VSyncMonitorAndroid::RegisterJNI(env);
  lynx::shell::LynxRuntimeWrapperAndroid::RegisterJNI(env);
  lynx::shell::LynxTemplateRendererAndroid::RegisterJni(env);
  lynx::shell::NativeFacadeAndroid::RegisterJni(env);
  lynx::shell::NativeFacadeReporterAndroid::RegisterJni(env);

#if MEMORY_TRACING
  lynx::base::MemoryTracerAndroid::RegisterJNIUtils(env);
#endif

  lynx::tasm::fluency::FluencySampleAndroid::RegisterJNI(env);
  return JNI_VERSION_1_6;
}

}  // namespace lynx
