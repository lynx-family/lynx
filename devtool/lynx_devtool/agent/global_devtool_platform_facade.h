// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_GLOBAL_DEVTOOL_PLATFORM_FACADE_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_GLOBAL_DEVTOOL_PLATFORM_FACADE_H_

#include <memory>
#include <string>

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "base/trace/native/trace_controller.h"
#endif

namespace lynx {
namespace devtool {

/*
Why use GlobalDevToolPlatformFacade?

Just like LynxGlobalDevToolMediator, we need a global facade that aligns with
the App's lifecycle. This is essential because some protocols, such as
Memory.XX, must be processed before the view is opened. Moreover, each platform
has its distinct implementation. To accommodate this, we implement the
GetInstance method and all other virtual methods, making this possible.

The call chain is as follows:
GlobalDevToolPlatformFacade::XX - the initial call in the chain
->
GlobalDevToolPlatformPlatform::xx - the next step in the chain, where the
operation or request is passed down to the platform-specific implementation.
->
static PlatformImpl::xx - This is the final step in the chain, where the actual
platform-specific code is executed.

*/

class GlobalDevToolPlatformFacade
    : public std::enable_shared_from_this<GlobalDevToolPlatformFacade> {
 public:
  static GlobalDevToolPlatformFacade& GetInstance();

  virtual ~GlobalDevToolPlatformFacade() = default;

  // The following functions are used for memory agent.
  virtual void StartMemoryTracing() = 0;
  virtual void StopMemoryTracing() = 0;

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  // The following functions are used for tracing agent.
  virtual lynx::trace::TraceController* GetTraceController() = 0;
  virtual lynx::trace::TracePlugin* GetFPSTracePlugin() = 0;
  virtual lynx::trace::TracePlugin* GetFrameViewTracePlugin() = 0;
  virtual lynx::trace::TracePlugin* GetInstanceTracePlugin() = 0;
  virtual std::string GetLynxVersion() { return ""; }
#endif

  virtual std::string GetSystemModelName() { return ""; }
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_GLOBAL_DEVTOOL_PLATFORM_FACADE_H_
