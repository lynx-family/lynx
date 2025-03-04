// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "devtool/lynx_devtool/tracing/platform/frame_trace_service_android.h"

#include "base/include/log/logging.h"
#include "core/base/android/jni_helper.h"
#include "devtool/lynx_devtool/build/gen/FrameTraceService_jni.h"

static void Initialize(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  auto* trace_service =
      reinterpret_cast<lynx::trace::FrameTraceServiceAndroid*>(nativePtr);
  if (trace_service) {
    trace_service->Initialize();
  }
}

static jlong CreateFrameTraceService(JNIEnv* env, jobject jcaller) {
  auto* trace_service = new lynx::trace::FrameTraceServiceAndroid(env, jcaller);
  return reinterpret_cast<jlong>(trace_service);
}

static void FPSTrace(JNIEnv* env, jobject jcaller, jlong nativePtr,
                     jlong startTime, jlong endTime) {
  auto* trace_service =
      reinterpret_cast<lynx::trace::FrameTraceServiceAndroid*>(nativePtr);
  if (trace_service) {
    trace_service->FPSTrace(static_cast<uint64_t>(startTime),
                            static_cast<uint64_t>(endTime));
  }
}

static void Screenshot(JNIEnv* env, jobject jcaller, jlong nativePtr,
                       jstring snapshot) {
  auto* trace_service =
      reinterpret_cast<lynx::trace::FrameTraceServiceAndroid*>(nativePtr);
  if (trace_service) {
    auto snapshot_str =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, snapshot);
    trace_service->Screenshot(snapshot_str);
  }
}

namespace lynx {
namespace trace {

bool FrameTraceServiceAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

FrameTraceServiceAndroid::FrameTraceServiceAndroid(JNIEnv* env, jobject owner)
    : weak_owner_(env, owner),
      frame_trace_service_(std::make_shared<lynx::trace::FrameTraceService>()) {
}

void FrameTraceServiceAndroid::Screenshot(const std::string& snapshot) {
  if (frame_trace_service_) {
    frame_trace_service_->SendScreenshots(snapshot);
  }
}

void FrameTraceServiceAndroid::FPSTrace(const uint64_t& startTime,
                                        const uint64_t& endTime) {
  if (frame_trace_service_) {
    frame_trace_service_->SendFPSData(startTime, endTime);
  }
}

void FrameTraceServiceAndroid::Initialize() {
  if (frame_trace_service_) {
    frame_trace_service_->Initialize();
  }
}
}  // namespace trace
}  // namespace lynx
#endif
