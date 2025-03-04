// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FRAME_TRACE_SERVICE_ANDROID_H_
#define DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FRAME_TRACE_SERVICE_ANDROID_H_

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "base/include/platform/android/scoped_java_ref.h"
#include "devtool/lynx_devtool/tracing/frame_trace_service.h"

namespace lynx {
namespace trace {

class FrameTraceServiceAndroid {
 public:
  static bool RegisterJNIUtils(JNIEnv *env);

  FrameTraceServiceAndroid(JNIEnv *env, jobject owner);
  ~FrameTraceServiceAndroid() = default;
  void Initialize();
  void Screenshot(const std::string &snapshot);
  void FPSTrace(const uint64_t &startTime, const uint64_t &endTime);

 private:
  FrameTraceServiceAndroid(const FrameTraceServiceAndroid &) = delete;
  FrameTraceServiceAndroid &operator=(const FrameTraceServiceAndroid &) =
      delete;

  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> weak_owner_;
  std::shared_ptr<lynx::trace::FrameTraceService> frame_trace_service_;
};

}  // namespace trace
}  // namespace lynx

#endif
#endif  // DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FRAME_TRACE_SERVICE_ANDROID_H_
