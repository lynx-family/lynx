// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_GLOBAL_DEVTOOL_PLATFORM_ANDROID_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_GLOBAL_DEVTOOL_PLATFORM_ANDROID_H_

#include <jni.h>

#include "base/include/no_destructor.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

namespace lynx {
namespace devtool {
class GlobalDevToolPlatformAndroid : public GlobalDevToolPlatformFacade {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);

  void StartMemoryTracing() override;
  void StopMemoryTracing() override;

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  // The following functions are used for tracing agent.
  lynx::trace::TraceController* GetTraceController() override;
  lynx::trace::TracePlugin* GetFPSTracePlugin() override;
  lynx::trace::TracePlugin* GetFrameViewTracePlugin() override;
  lynx::trace::TracePlugin* GetInstanceTracePlugin() override;
  std::string GetLynxVersion() override;
#endif

  std::string GetSystemModelName() override;

  GlobalDevToolPlatformAndroid(const GlobalDevToolPlatformAndroid&) = delete;
  GlobalDevToolPlatformAndroid& operator=(const GlobalDevToolPlatformAndroid&) =
      delete;
  GlobalDevToolPlatformAndroid(GlobalDevToolPlatformAndroid&&) = delete;
  GlobalDevToolPlatformAndroid& operator=(GlobalDevToolPlatformAndroid&&) =
      delete;
  ~GlobalDevToolPlatformAndroid() override = default;

 private:
  GlobalDevToolPlatformAndroid() = default;

  friend class base::NoDestructor<GlobalDevToolPlatformAndroid>;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_GLOBAL_DEVTOOL_PLATFORM_ANDROID_H_
