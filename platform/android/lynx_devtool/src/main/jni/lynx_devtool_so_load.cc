// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/android/message_loop_android.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/logging_android.h"
#include "devtool/lynx_devtool/agent/android/devtool_platform_android.h"
#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"
#include "devtool/lynx_devtool/android/devtool_message_handler_android.h"
#include "devtool/lynx_devtool/android/invoke_cdp_from_sdk_sender_android.h"
#include "devtool/lynx_devtool/android/lynx_devtool_ng_android.h"
#include "devtool/lynx_devtool/common/android/lynx_inspector_owner_native_glue.h"
#include "devtool/lynx_devtool/recorder/android/recorder_controller_native_glue.h"
#include "devtool/lynx_devtool/tracing/platform/fps_trace_plugin_android.h"
#include "devtool/lynx_devtool/tracing/platform/frame_trace_service_android.h"
#include "devtool/lynx_devtool/tracing/platform/frameview_trace_plugin_android.h"
#include "devtool/lynx_devtool/tracing/platform/instance_trace_plugin_android.h"

namespace lynx {

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  lynx::base::android::InitVM(vm);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::fml::MessageLoopAndroid::Register(env);
  lynx::devtool::LynxDevToolNGAndroid::RegisterJNIUtils(env);
  lynx::devtool::DevToolMessageHandlerAndroid::RegisterJNIUtils(env);
  lynx::devtool::InvokeCDPFromSDKSenderAndroid::RegisterJNIUtils(env);
  lynx::devtool::DevToolPlatformAndroid::RegisterJNIUtils(env);
  lynx::devtool::GlobalDevToolPlatformAndroid::RegisterJNIUtils(env);
  lynx::devtool::LynxInspectorOwnerNativeGlue::RegisterJNIUtils(env);
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  lynx::trace::FrameTraceServiceAndroid::RegisterJNIUtils(env);
  lynx::trace::FPSTracePluginAndroid::RegisterJNIUtils(env);
  lynx::trace::FrameViewTracePluginAndroid::RegisterJNIUtils(env);
  lynx::trace::InstanceTracePluginAndroid::RegisterJNIUtils(env);
#endif
  lynx::devtool::RecorderControllerNativeGlue::RegisterJNIUtils(env);

  lynx::base::logging::RegisterJNI(env);

  lynx::base::logging::InitLynxLog();

  return JNI_VERSION_1_6;
}

}  // namespace lynx
