// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_STARLIGHT_STANDALONE_ANDROID_STARLIGHT_ANDROID_H_
#define CORE_SERVICES_STARLIGHT_STANDALONE_ANDROID_STARLIGHT_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/services/starlight_standalone/core/include/starlight.h"

namespace starlight {
class StarlightAndroid {
 public:
  static bool RegisterJNIUtils(JNIEnv *env);
};

class SLMeasureDelegateAndroid : public MeasureDelegate {
 public:
  SLMeasureDelegateAndroid(JNIEnv *env, jobject obj);
  ~SLMeasureDelegateAndroid() override;
  SLSize Measure(SLConstraints &constraint) override;
  float Baseline(SLConstraints &constraint) override;
  void Alignment() override;

 private:
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
};
}  // namespace starlight

#endif  // CORE_SERVICES_STARLIGHT_STANDALONE_ANDROID_STARLIGHT_ANDROID_H_
