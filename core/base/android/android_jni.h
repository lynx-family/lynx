// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_ANDROID_JNI_H_
#define CORE_BASE_ANDROID_ANDROID_JNI_H_

#include <jni.h>
#include <cstdint>

#include "base/include/base_export.h"
#include "base/include/platform/android/jni_utils.h"
#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace base {
namespace android {

BASE_EXPORT_FOR_DEVTOOL void CheckException(JNIEnv *env);

// Used to indicate whether there is an jni exception after a jni call.
// It should be noted that only the result checked by calling this method
// immediately after JNI invocation is valid.
bool &HasJNIException();

// Get message and stack of a throwable, and append the results to
// parameters error_msg and error_stack respectively
void GetExceptionInfo(JNIEnv *env, lynx::base::android::ScopedLocalJavaRef<jthrowable> &throwable,
    std::string &error_msg, std::string &error_stack);

int GetJNILocalFrameCapacity();
class JniLocalScope {
 public:
  JniLocalScope(JNIEnv *env, jint capacity = 256) : env_(env) {
    hasFrame_ = false;

    for (size_t i = capacity; i > 0; i /= 2) {
      auto pushResult = env->PushLocalFrame(i);
      if (pushResult == 0) {
        hasFrame_ = true;
        break;
      }

      // if failed, clear the exception and try again with less capacity
      if (pushResult < 0) {
        jthrowable java_throwable = env->ExceptionOccurred();
        if (java_throwable) {
          env->ExceptionClear();
          env->DeleteLocalRef(java_throwable);
        }
      }
    }
  }

  ~JniLocalScope() {
    if (hasFrame_) {
      env_->PopLocalFrame(nullptr);
    }
  }

 private:
  JNIEnv *env_;
  bool hasFrame_;
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_ANDROID_JNI_H_
