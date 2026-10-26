// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/lynx_error_android.h"

#include "base/include/debug/lynx_error.h"
#include "core/base/android/jni_helper.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxError_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxError_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxError(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace base {
namespace android {

LynxErrorAndroid::LynxErrorAndroid(const base::LynxError& error) {
  JNIEnv* env = AttachCurrentThread();
  auto j_msg =
      JNIConvertHelper::ConvertToJNIStringUTF(env, error.error_message_);
  auto j_root_cause =
      JNIConvertHelper::ConvertToJNIStringUTF(env, error.root_cause_);
  auto j_suggestion =
      JNIConvertHelper::ConvertToJNIStringUTF(env, error.fix_suggestion_);
  auto j_level = JNIConvertHelper::ConvertToJNIStringUTF(
      env, LynxError::GetLevelString(static_cast<int32_t>(error.error_level_)));
  auto j_map = JNIHelper::ConvertSTLStringMapToJavaMap(env, error.custom_info_);

  jni_object_.Reset(env, Java_LynxError_createLynxError(
                             env, error.error_code_, j_msg.Get(),
                             j_root_cause.Get(), j_suggestion.Get(),
                             j_level.Get(), j_map.Get(), error.is_logbox_only_)
                             .Get());
}

}  // namespace android
}  // namespace base
}  // namespace lynx
