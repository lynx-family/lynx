// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/feature_count/android/feature_counter_android.h"

#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "core/build/gen/LynxFeatureCounter_jni.h"
#include "core/services/feature_count/global_feature_counter.h"

void FeatureCount(JNIEnv* env, jclass jcaller, jint feature, jint instanceId) {
  lynx::tasm::report::GlobalFeatureCounter::Count(
      static_cast<lynx::tasm::report::LynxFeature>(feature), instanceId);
}

namespace lynx {
namespace tasm {
namespace report {

void RegisterJniFeatureCounter(JNIEnv* env) { (void)RegisterNativesImpl(env); }

}  // namespace report
}  // namespace tasm
}  // namespace lynx
