// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_BASE_ANDROID_CALLSTACK_UTIL_ANDROID_H_
#define CORE_BASE_ANDROID_CALLSTACK_UTIL_ANDROID_H_
#include <jni.h>

#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/base/lynx_export.h"

namespace lynx {
namespace base {
namespace android {

class CallStackUtilAndroid {
 public:
  LYNX_EXPORT_FOR_DEVTOOL static std::string GetMessageOfCauseChain(
      JNIEnv* env, const ScopedLocalJavaRef<jthrowable>& throwable);
  LYNX_EXPORT_FOR_DEVTOOL static std::string GetStackTraceStringWithLineTrimmed(
      JNIEnv* env, const ScopedLocalJavaRef<jthrowable>& throwable);
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_CALLSTACK_UTIL_ANDROID_H_
