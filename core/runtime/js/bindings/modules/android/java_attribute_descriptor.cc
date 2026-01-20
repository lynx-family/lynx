// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/android/java_attribute_descriptor.h"

#include "platform/android/lynx_android/src/main/jni/gen/AttributeDescriptor_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/AttributeDescriptor_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForAttributeDescriptor(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace runtime {
namespace js {
std::string JavaAttributeDescriptor::getName() {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> obj =
      Java_AttributeDescriptor_getName(env, wrapper_.Get());
  const char* str = env->GetStringUTFChars(obj.Get(), JNI_FALSE);
  return str;
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JavaAttributeDescriptor::getValue() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_AttributeDescriptor_getValue(env, wrapper_.Get());
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
