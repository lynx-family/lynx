// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/platform/android/src/main/jni/gen/LynxBaseEnv_jni.h"
#include "base/platform/android/src/main/jni/gen/LynxBaseEnv_register_jni.h"
#include "base/src/application_directory/application_directory_impl.h"

namespace lynxbase {
namespace jni {
bool RegisterJNIForLynxBaseEnv(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynxbase

static void SetApplicationDataDirectory(JNIEnv* env, jclass,
                                        jstring directory) {
  lynx::base::SetApplicationDataDirectory(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, directory));
}
