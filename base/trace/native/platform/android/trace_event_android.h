// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_PLATFORM_ANDROID_TRACE_EVENT_ANDROID_H_
#define BASE_TRACE_NATIVE_PLATFORM_ANDROID_TRACE_EVENT_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace trace {
class TraceEventAndroid {
 public:
  static bool RegisterJNI(JNIEnv* env);

 private:
  TraceEventAndroid(const TraceEventAndroid&) = delete;
  TraceEventAndroid& operator=(const TraceEventAndroid&) = delete;
};

}  // namespace trace
}  // namespace lynx
#endif  // BASE_TRACE_NATIVE_PLATFORM_ANDROID_TRACE_EVENT_ANDROID_H_
