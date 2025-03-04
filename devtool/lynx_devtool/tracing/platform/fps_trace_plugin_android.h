// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FPS_TRACE_PLUGIN_ANDROID_H_
#define DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FPS_TRACE_PLUGIN_ANDROID_H_

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "base/include/platform/android/scoped_java_ref.h"
#include "base/trace/native/trace_controller.h"

namespace lynx {
namespace trace {

class FPSTracePluginAndroid : public TracePlugin {
 public:
  static bool RegisterJNIUtils(JNIEnv *env);

  FPSTracePluginAndroid(JNIEnv *env, jobject owner) : weak_owner_(env, owner){};
  virtual ~FPSTracePluginAndroid() = default;
  virtual void DispatchBegin() override;
  virtual void DispatchEnd() override;
  virtual std::string Name() override;

 private:
  FPSTracePluginAndroid(const FPSTracePluginAndroid &) = delete;
  FPSTracePluginAndroid &operator=(const FPSTracePluginAndroid &) = delete;
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> weak_owner_;
};

}  // namespace trace
}  // namespace lynx

#endif
#endif  // DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FPS_TRACE_PLUGIN_ANDROID_H_
