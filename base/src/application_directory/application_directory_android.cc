// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/application_directory.h"
#include "base/include/platform/android/jni_convert_helper.h"
#include "base/platform/android/src/main/jni/gen/LynxBaseEnv_jni.h"
#include "base/platform/android/src/main/jni/gen/LynxBaseEnv_register_jni.h"

namespace lynxbase {
namespace jni {
bool RegisterJNIForLynxBaseEnv(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynxbase

static jbyteArray GetApplicationDataDirectory(JNIEnv* env, jclass) {
  const auto directory = lynx::base::GetApplicationDataDirectory();
  return lynx::base::android::JNIConvertHelper::ConvertToJNIByteArray(env,
                                                                      directory)
      .Release();
}

static void SetApplicationDataDirectory(JNIEnv* env, jclass,
                                        jstring directory) {
  lynx::base::SetApplicationDataDirectory(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, directory));
}

static jbyteArray GetApplicationDataSubDirectory(JNIEnv* env, jclass,
                                                 jstring name) {
  const auto directory = lynx::base::GetApplicationDataSubDirectory(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name));
  return lynx::base::android::JNIConvertHelper::ConvertToJNIByteArray(env,
                                                                      directory)
      .Release();
}
