// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_PLATFORM_ANDROID_JNI_UTILS_H_
#define BASE_INCLUDE_PLATFORM_ANDROID_JNI_UTILS_H_

#include <jni.h>

#include <cstdint>
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace base {
namespace android {

void InitVM(JavaVM *vm);

JNIEnv *AttachCurrentThread();

void DetachFromVM();

ScopedLocalJavaRef<jclass> GetClass(JNIEnv *env, const char *class_name);

ScopedGlobalJavaRef<jclass> GetGlobalClass(JNIEnv *env, const char *class_name);

enum MethodType {
  STATIC_METHOD,
  INSTANCE_METHOD,
};

jmethodID GetMethod(JNIEnv *env, jclass clazz, MethodType type,
                    const char *method_name, const char *jni_signature);

jmethodID GetMethod(JNIEnv *env, jclass clazz, MethodType type,
                    const char *method_name, const char *jni_signature,
                    intptr_t *method_id);

bool HasException(JNIEnv *env);
bool ClearException(JNIEnv *env);
// Return false if exception occured
bool CheckException(JNIEnv *env, std::string &exception_msg);
bool CheckAndPrintException(JNIEnv *env);
std::string GetJavaExceptionInfo(JNIEnv *env, jthrowable java_throwable);

int GetAPILevel();

}  // namespace android
}  // namespace base
}  // namespace lynx

namespace fml {
namespace jni {
using lynx::base::android::AttachCurrentThread;
using lynx::base::android::CheckException;
using lynx::base::android::ClearException;
using lynx::base::android::DetachFromVM;
using lynx::base::android::GetAPILevel;
using lynx::base::android::HasException;
}  // namespace jni
}  // namespace fml

#endif  // BASE_INCLUDE_PLATFORM_ANDROID_JNI_UTILS_H_
