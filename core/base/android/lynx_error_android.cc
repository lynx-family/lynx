// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/lynx_error_android.h"

#include "base/include/debug/lynx_error.h"
#include "core/base/android/jni_helper.h"
#include "core/build/gen/LynxError_jni.h"

namespace lynx {
namespace base {
namespace android {

bool LynxErrorAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

LynxErrorAndroid::LynxErrorAndroid(
    int error_code, const std::string& error_message,
    const std::string& fix_suggestion, base::LynxErrorLevel level,
    const std::unordered_map<std::string, std::string>& custom_info,
    bool is_logbox_only) {
  JNIEnv* env = AttachCurrentThread();
  auto j_msg = JNIConvertHelper::ConvertToJNIStringUTF(env, error_message);
  auto j_suggestion =
      JNIConvertHelper::ConvertToJNIStringUTF(env, fix_suggestion);
  auto j_level = JNIConvertHelper::ConvertToJNIStringUTF(
      env, LynxError::GetLevelString(static_cast<int32_t>(level)));
  auto j_map = JNIHelper::ConvertSTLStringMapToJavaMap(env, custom_info);

  jni_object_.Reset(env, Java_LynxError_createLynxError(
                             env, error_code, j_msg.Get(), j_suggestion.Get(),
                             j_level.Get(), j_map.Get(), is_logbox_only)
                             .Get());
}

}  // namespace android
}  // namespace base
}  // namespace lynx
