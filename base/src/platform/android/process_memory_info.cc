// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/android/process_memory_info.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "base/platform/android/src/main/jni/gen/ProcessMemoryInfo_jni.h"
#include "base/platform/android/src/main/jni/gen/ProcessMemoryInfo_register_jni.h"

namespace lynxbase {
namespace jni {
bool RegisterJNIForProcessMemoryInfo(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynxbase

namespace lynx {
namespace base {
namespace android {

std::vector<std::string> GetProcessMemoryInfo() {
  JNIEnv* env = AttachCurrentThread();
  auto memory_stats = Java_ProcessMemoryInfo_getMemoryStats(env);
  return JNIConvertHelper::ConvertJavaStringArrayToStringVector(
      env, memory_stats.Get());
}

}  // namespace android
}  // namespace base
}  // namespace lynx
