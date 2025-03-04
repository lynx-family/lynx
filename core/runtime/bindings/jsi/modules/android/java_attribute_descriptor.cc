// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/java_attribute_descriptor.h"

#include "core/build/gen/AttributeDescriptor_jni.h"

namespace lynx {
namespace piper {
bool JavaAttributeDescriptor::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

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

}  // namespace piper
}  // namespace lynx
