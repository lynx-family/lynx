// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_LYNX_JSI_OBJECT_DESCRIPTOR_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_LYNX_JSI_OBJECT_DESCRIPTOR_H_

#include <optional>
#include <string>
#include <utility>

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace piper {

/**
 * Class to hold java JSIObjectDescriptor, call jni function to get
 * JSPropertyDescriptorInfo
 */
class LynxJSIObjectDescriptor {
 public:
  static bool RegisterJNI(JNIEnv* env);

  explicit LynxJSIObjectDescriptor(
      base::android::ScopedGlobalJavaRef<jobject> jsi_object_descriptor)
      : jsi_object_descriptor_(std::move(jsi_object_descriptor)) {}
  ~LynxJSIObjectDescriptor() = default;
  LynxJSIObjectDescriptor(const LynxJSIObjectDescriptor&) = delete;
  LynxJSIObjectDescriptor& operator=(const LynxJSIObjectDescriptor&) = delete;

  std::optional<std::string> GetJSPropertyDescriptorInfo(
      JNIEnv* env, const std::string& field_name);

 private:
  base::android::ScopedGlobalJavaRef<jobject> jsi_object_descriptor_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_LYNX_JSI_OBJECT_DESCRIPTOR_H_
