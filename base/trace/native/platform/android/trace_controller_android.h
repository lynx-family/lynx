// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_PLATFORM_ANDROID_TRACE_CONTROLLER_ANDROID_H_
#define BASE_TRACE_NATIVE_PLATFORM_ANDROID_TRACE_CONTROLLER_ANDROID_H_

#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "base/trace/native/trace_controller.h"

namespace lynx {
namespace trace {

class TraceControllerDelegateAndroid : public TraceController::Delegate {
 public:
  static bool RegisterJNIUtils(JNIEnv *env);

  TraceControllerDelegateAndroid(JNIEnv *env, jobject owner)
      : weak_owner_(env, owner){};

  ~TraceControllerDelegateAndroid() = default;

  std::string GenerateTracingFileDir() override;

  void RefreshATraceTags() override;

 private:
  TraceControllerDelegateAndroid(const TraceControllerDelegateAndroid &) =
      delete;

  TraceControllerDelegateAndroid &operator=(
      const TraceControllerDelegateAndroid &) = delete;

  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> weak_owner_;
};

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_PLATFORM_ANDROID_TRACE_CONTROLLER_ANDROID_H_
