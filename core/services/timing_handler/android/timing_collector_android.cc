// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/android/timing_collector_android.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/build/gen/TimingCollector_jni.h"

jlong CreateTimingCollector(JNIEnv* env, jobject jcaller) {
  auto* timing_collector = new lynx::tasm::timing::TimingCollectorAndroid();
  auto* sp_timing_collector =
      new std::shared_ptr<lynx::tasm::timing::TimingCollectorAndroid>(
          timing_collector);
  return reinterpret_cast<jlong>(sp_timing_collector);
}

void ReleaseTimingCollector(JNIEnv* env, jobject jcaller, jlong ptr) {
  std::shared_ptr<lynx::tasm::timing::TimingCollectorAndroid>*
      timing_collector = reinterpret_cast<
          std::shared_ptr<lynx::tasm::timing::TimingCollectorAndroid>*>(ptr);
  delete timing_collector;
}

void MarkDrawEndTimingIfNeeded(JNIEnv* env, jobject jcaller, jlong native_ptr) {
  if (!native_ptr) {
    return;
  }
  auto* timing_collector = reinterpret_cast<
      std::shared_ptr<lynx::tasm::timing::TimingCollectorAndroid>*>(native_ptr);
  if (!timing_collector || !timing_collector->get()) {
    return;
  }
  timing_collector->get()->MarkDrawEndTimingIfNeeded();
}

void SetTiming(JNIEnv* env, jobject jcaller, jlong native_ptr,
               jstring pipeline_id, jstring timing_key, jlong us_timestamp) {
  if (!native_ptr) {
    return;
  }
  auto* timing_collector = reinterpret_cast<
      std::shared_ptr<lynx::tasm::timing::TimingCollectorAndroid>*>(native_ptr);
  if (!timing_collector || !timing_collector->get()) {
    return;
  }
  auto pipeline_id_str = lynx::base::android::JNIConvertHelper::ConvertToString(
      env, reinterpret_cast<jstring>(pipeline_id));
  auto timing_key_str = lynx::base::android::JNIConvertHelper::ConvertToString(
      env, reinterpret_cast<jstring>(timing_key));
  timing_collector->get()->SetTiming(pipeline_id_str, timing_key_str,
                                     us_timestamp);
}

namespace lynx {
namespace tasm {
namespace timing {

bool TimingCollectorAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
