// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"

#include <sys/system_properties.h>

#include "core/base/android/jni_helper.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"
#include "devtool/lynx_devtool/build/gen/GlobalDevToolPlatformAndroidDelegate_jni.h"

namespace lynx {
namespace devtool {

GlobalDevToolPlatformFacade& GlobalDevToolPlatformFacade::GetInstance() {
  static base::NoDestructor<GlobalDevToolPlatformAndroid> instance;
  return *(instance.get());
}

bool GlobalDevToolPlatformAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void GlobalDevToolPlatformAndroid::StartMemoryTracing() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_GlobalDevToolPlatformAndroidDelegate_startMemoryTracing(env);
}

void GlobalDevToolPlatformAndroid::StopMemoryTracing() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_GlobalDevToolPlatformAndroidDelegate_stopMemoryTracing(env);
}

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
lynx::trace::TraceController*
GlobalDevToolPlatformAndroid::GetTraceController() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  intptr_t res =
      Java_GlobalDevToolPlatformAndroidDelegate_getTraceController(env);
  return reinterpret_cast<lynx::trace::TraceController*>(res);
}

lynx::trace::TracePlugin* GlobalDevToolPlatformAndroid::GetFPSTracePlugin() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  intptr_t res =
      Java_GlobalDevToolPlatformAndroidDelegate_getFPSTracePlugin(env);
  return reinterpret_cast<lynx::trace::TracePlugin*>(res);
}

lynx::trace::TracePlugin*
GlobalDevToolPlatformAndroid::GetFrameViewTracePlugin() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  intptr_t res =
      Java_GlobalDevToolPlatformAndroidDelegate_getFrameViewTracePlugin(env);
  return reinterpret_cast<lynx::trace::TracePlugin*>(res);
}

lynx::trace::TracePlugin*
GlobalDevToolPlatformAndroid::GetInstanceTracePlugin() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  intptr_t res =
      Java_GlobalDevToolPlatformAndroidDelegate_getInstanceTracePlugin(env);
  return reinterpret_cast<lynx::trace::TracePlugin*>(res);
}

std::string GlobalDevToolPlatformAndroid::GetLynxVersion() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto lynx_version =
      Java_GlobalDevToolPlatformAndroidDelegate_getLynxVersion(env);
  return lynx::base::android::JNIConvertHelper::ConvertToString(
      env, lynx_version.Get());
}
#endif

std::string GlobalDevToolPlatformAndroid::GetSystemModelName() {
  char value[PROP_VALUE_MAX] = {0};
  int ret = __system_property_get("ro.product.model", value);
  if (ret <= 0) {
    return "";
  }
  return value;
}

}  // namespace devtool
}  // namespace lynx
