// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "devtool/lynx_devtool/tracing/platform/fps_trace_plugin_android.h"

#include "core/base/android/jni_helper.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/FPSTrace_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/FPSTrace_register_jni.h"

namespace lynx {
namespace devtool {
namespace jni {

bool RegisterJNIForFPSTrace(JNIEnv* env) { return RegisterNativesImpl(env); }

}  // namespace jni
}  // namespace devtool
}  // namespace lynx

static jlong CreateFPSTrace(JNIEnv* env, jobject jcaller) {
  auto* trace_plugin = new lynx::trace::FPSTracePluginAndroid(env, jcaller);
  return reinterpret_cast<jlong>(trace_plugin);
}

namespace lynx {
namespace trace {

void FPSTracePluginAndroid::DispatchBegin() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_FPSTrace_startFPSTrace(env, weak_owner_.Get());
}

void FPSTracePluginAndroid::DispatchEnd() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_FPSTrace_stopFPSTrace(env, weak_owner_.Get());
}

std::string FPSTracePluginAndroid::Name() { return "FPS"; }

}  // namespace trace
}  // namespace lynx
#endif
