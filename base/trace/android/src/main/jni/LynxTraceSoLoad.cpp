// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <android/log.h>
#include <jni.h>

#include <string>

#include "base/include/fml/platform/android/message_loop_android.h"
#include "base/include/platform/android/jni_utils.h"
#include "base/trace/native/platform/android/trace_controller_android.h"
#include "base/trace/native/platform/android/trace_event_android.h"

namespace lynx {
namespace trace {
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  lynx::base::android::InitVM(vm);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  fml::MessageLoopAndroid::Register(env);
  TraceEventAndroid::RegisterJNI(env);
  TraceControllerDelegateAndroid::RegisterJNIUtils(env);
  return JNI_VERSION_1_6;
}
}  // namespace trace
}  // namespace lynx
