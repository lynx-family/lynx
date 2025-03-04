// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_JAVA_METHOD_DESCRIPTOR_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_JAVA_METHOD_DESCRIPTOR_H_

#include <functional>
#include <optional>
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace piper {

class JavaMethodDescriptor {
 public:
  static bool RegisterJNI(JNIEnv* env);

  JavaMethodDescriptor(JNIEnv* env, jobject jni_object)
      : wrapper_(env, jni_object) {}
  std::string getName();
  std::string getSignature();
  lynx::base::android::ScopedLocalJavaRef<jobject> getMethod();

 private:
  base::android::ScopedGlobalJavaRef<jobject> wrapper_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_JAVA_METHOD_DESCRIPTOR_H_
