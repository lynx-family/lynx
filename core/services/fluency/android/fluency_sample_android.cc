// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/fluency/android/fluency_sample_android.h"

#include <stdlib.h>

#include "core/build/gen/FluencySample_jni.h"
#include "core/services/fluency/fluency_tracer.h"
void SetFluencySample(JNIEnv* env, jclass jcaller, jboolean enable) {
  lynx::tasm::FluencyTracer::SetForceEnable(enable);
}
void NeedCheckFluencyEnable(JNIEnv* env, jclass jcaller) {
  lynx::tasm::FluencyTracer::SetNeedCheck();
}
namespace lynx {
namespace tasm {
namespace fluency {

bool FluencySampleAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace fluency
}  // namespace tasm
}  // namespace lynx
