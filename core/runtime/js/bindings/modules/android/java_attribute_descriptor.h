// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_JAVA_ATTRIBUTE_DESCRIPTOR_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_JAVA_ATTRIBUTE_DESCRIPTOR_H_

#include <functional>
#include <optional>
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace runtime {
namespace js {
class JavaAttributeDescriptor {
 public:
  JavaAttributeDescriptor() {}
  JavaAttributeDescriptor(JNIEnv* env, jobject jni_object)
      : wrapper_(env, jni_object) {}
  std::string getName();
  lynx::base::android::ScopedLocalJavaRef<jobject> getValue();

 private:
  base::android::ScopedGlobalJavaRef<jobject> wrapper_;
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_JAVA_ATTRIBUTE_DESCRIPTOR_H_
