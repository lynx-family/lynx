// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/android/platform_jsi/lynx_jsi_object_descriptor.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "platform/android/lynx_android/src/main/jni/gen/ILynxJSIObjectDescriptor_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/ILynxJSIObjectDescriptor_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForILynxJSIObjectDescriptor(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace runtime {
namespace js {
std::vector<std::string> LynxJSIObjectDescriptor::GetJSPropertyDescriptorInfo(
    JNIEnv* env, const std::string& field_name) {
  if (jsi_object_descriptor_.IsNull()) {
    return {};
  }
  auto j_field_name =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, field_name);
  auto j_field_info = Java_ILynxJSIObjectDescriptor_getLynxObjectDescriptorInfo(
      env, jsi_object_descriptor_.Get(), j_field_name.Get());
  return base::android::JNIConvertHelper::ConvertJavaStringArrayToStringVector(
      env, j_field_info.Get());
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
