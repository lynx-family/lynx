// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/platform_jsi/lynx_jsi_object_descriptor.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "build/gen/ILynxJSIObjectDescriptor_jni.h"

namespace lynx {
namespace piper {

bool LynxJSIObjectDescriptor::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

std::optional<std::string> LynxJSIObjectDescriptor::GetJSPropertyDescriptorInfo(
    JNIEnv* env, const std::string& field_name) {
  auto j_field_name =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, field_name);
  auto j_field_info = Java_ILynxJSIObjectDescriptor_getLynxObjectDescriptorInfo(
      env, jsi_object_descriptor_.Get(), j_field_name.Get());
  return j_field_info.IsNull()
             ? std::nullopt
             : std::optional<std::string>{
                   base::android::JNIConvertHelper::ConvertToString(
                       env, j_field_info.Get())};
}

}  // namespace piper
}  // namespace lynx
