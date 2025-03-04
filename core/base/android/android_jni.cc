// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/android_jni.h"

#include <sys/prctl.h>

#include <string>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "core/base/android/callstack_util_android.h"
#include "core/build/gen/lynx_sub_error_code.h"

namespace lynx {
namespace base {
namespace android {

void GetExceptionInfo(
    JNIEnv *env, lynx::base::android::ScopedLocalJavaRef<jthrowable> &throwable,
    std::string &error_msg, std::string &error_stack) {
  if (!throwable.Get()) {
    return;
  }
  error_msg.append(
      CallStackUtilAndroid::GetMessageOfCauseChain(env, throwable));
  error_stack.append(
      CallStackUtilAndroid::GetStackTraceStringWithLineTrimmed(env, throwable));
}

void CheckException(JNIEnv *env) {
  HasJNIException() = false;
  if (!HasException(env)) {
    return;
  }

  static thread_local bool is_reentering = false;

  // Exception has been found, might as well tell BreakPad about it.
  lynx::base::android::ScopedLocalJavaRef<jthrowable> throwable(
      env, env->ExceptionOccurred());
  if (throwable.Get()) {
    // Clear the pending exception, since a local reference is now held.
    env->ExceptionDescribe();
    env->ExceptionClear();

    // If is reentering, ignore the exception thrown by
    // GetExceptionInfo to avoid infinite recursion
    if (!is_reentering) {
      is_reentering = true;
      std::string error_message;
      std::string error_stack;
      GetExceptionInfo(env, throwable, error_message, error_stack);

      lynx::base::LynxError error{error::E_EXCEPTION_JNI,
                                  std::move(error_message)};
      error.custom_info_.emplace("error_stack", std::move(error_stack));
      lynx::base::ErrorStorage::GetInstance().SetError(std::move(error));
      is_reentering = false;
    }
    HasJNIException() = true;
  }

  // Now, feel good about it and die.
  // CHECK(false) << "Please include Java exception stack in crash report";
}

// Used to indicate whether there is
// an jni exception after a jni call
bool &HasJNIException() {
  static thread_local bool has_jni_exception = false;
  return has_jni_exception;
}

int GetJNILocalFrameCapacity() {
  JNIEnv *env = base::android::AttachCurrentThread();
  int capacity = 1;
  for (; capacity < 512; capacity++) {
    auto pushResult = env->PushLocalFrame(capacity);

    if (pushResult < 0) {
      jthrowable java_throwable = env->ExceptionOccurred();
      if (java_throwable) {
        // Clear the pending exception, since a local reference is now held.
        // env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(java_throwable);
      }

      env->PopLocalFrame(nullptr);
      return capacity - 1;
    }
  }

  return capacity;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
