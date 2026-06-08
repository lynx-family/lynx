// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_GLOBAL_DEVTOOL_PLATFORM_FACADE_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_GLOBAL_DEVTOOL_PLATFORM_FACADE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "base/include/closure.h"
#include "base/trace/native/trace_controller.h"

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
  // The platform completes Memory.getAllMemoryUsage exactly once.
  //
  // result_json must be a JSON object encoded as a string. error_message takes
  // precedence when it is non-empty, so callers do not need to parse partial or
  // platform-specific failure payloads. MoveOnlyClosure keeps the ownership
  // model explicit for platform bridges that capture request-scoped resources.
  using MemoryUsageCallback =
      base::MoveOnlyClosure<void, const std::string&, const std::string&>;

  static GlobalDevToolPlatformFacade& GetInstance();

  virtual ~GlobalDevToolPlatformFacade() = default;

  // The following functions are used for memory agent.
  virtual void StartMemoryTracing() = 0;
  virtual void StopMemoryTracing() = 0;
  virtual void GetAllMemoryUsage(int64_t timeout_ms,
                                 MemoryUsageCallback callback) {
    (void)timeout_ms;
    if (callback) {
      std::move(callback)(
          "{}",
          "Memory.getAllMemoryUsage is not available in this Lynx runtime.");
    }
  }

  // The following functions are used for tracing agent.
  virtual lynx::trace::TraceController* GetTraceController() = 0;
  virtual lynx::trace::TracePlugin* GetFPSTracePlugin() = 0;
  virtual lynx::trace::TracePlugin* GetFrameViewTracePlugin() = 0;
  virtual lynx::trace::TracePlugin* GetInstanceTracePlugin() = 0;
  virtual std::string GetLynxVersion() { return ""; }

  virtual std::string GetSystemModelName() { return ""; }
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_GLOBAL_DEVTOOL_PLATFORM_FACADE_H_
