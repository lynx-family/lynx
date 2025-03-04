// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FLUENCY_ANDROID_FLUENCY_SAMPLE_ANDROID_H_
#define CORE_SERVICES_FLUENCY_ANDROID_FLUENCY_SAMPLE_ANDROID_H_
#include <jni.h>
#include <stdlib.h>

namespace lynx {
namespace tasm {
namespace fluency {
class FluencySampleAndroid {
 public:
  static bool RegisterJNI(JNIEnv* env);
};

}  // namespace fluency
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FLUENCY_ANDROID_FLUENCY_SAMPLE_ANDROID_H_
