// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_NATIVE_FACADE_REPORTER_ANDROID_H_
#define CORE_SHELL_ANDROID_NATIVE_FACADE_REPORTER_ANDROID_H_

#include <memory>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/shell/native_facade_reporter.h"

namespace lynx {
namespace shell {

class NativeFacadeReporterAndroid : public NativeFacadeReporter {
 public:
  NativeFacadeReporterAndroid(JNIEnv* env, jobject jni_object)
      : jni_object_(env, jni_object) {}
  ~NativeFacadeReporterAndroid() override = default;

  NativeFacadeReporterAndroid(const NativeFacadeReporterAndroid& facade) =
      delete;
  NativeFacadeReporterAndroid& operator=(const NativeFacadeReporterAndroid&) =
      delete;

  NativeFacadeReporterAndroid(NativeFacadeReporterAndroid&& facade) = default;
  NativeFacadeReporterAndroid& operator=(NativeFacadeReporterAndroid&&) =
      default;

  static void RegisterJni(JNIEnv* env);

  void OnPerformanceEvent(const lepus::Value& entry) override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_NATIVE_FACADE_REPORTER_ANDROID_H_
