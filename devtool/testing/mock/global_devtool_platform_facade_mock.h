// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_GLOBAL_DEVTOOL_PLATFORM_FACADE_MOCK_H_
#define DEVTOOL_TESTING_MOCK_GLOBAL_DEVTOOL_PLATFORM_FACADE_MOCK_H_

#include <string>
#include <utility>

#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

namespace lynx {
namespace testing {

class GlobalDevToolPlatformFacadeMock
    : public lynx::devtool::GlobalDevToolPlatformFacade {
 public:
  static std::string& MemoryUsageResultJson() {
    static std::string result_json = "{}";
    return result_json;
  }

  static std::string& MemoryUsageErrorMessage() {
    static std::string error_message;
    return error_message;
  }

  static int64_t& LastMemoryUsageTimeoutMs() {
    static int64_t timeout_ms = -1;
    return timeout_ms;
  }

  void StartMemoryTracing() override {}
  void StopMemoryTracing() override {}
  void GetAllMemoryUsage(
      int64_t timeout_ms,
      lynx::devtool::GlobalDevToolPlatformFacade::MemoryUsageCallback callback)
      override {
    // Tests mutate the static payloads above before dispatching a CDP command.
    // The mock immediately completes through the same callback path used by
    // real platform bridges, which keeps mediator parsing and error handling
    // covered without introducing platform dependencies.
    LastMemoryUsageTimeoutMs() = timeout_ms;
    if (callback) {
      std::move(callback)(MemoryUsageResultJson(), MemoryUsageErrorMessage());
    }
  }
  lynx::trace::TraceController* GetTraceController() override {
    return nullptr;
  }
  lynx::trace::TracePlugin* GetFPSTracePlugin() override { return nullptr; }
  lynx::trace::TracePlugin* GetFrameViewTracePlugin() override {
    return nullptr;
  }
  lynx::trace::TracePlugin* GetInstanceTracePlugin() override {
    return nullptr;
  }
};

}  // namespace testing

}  // namespace lynx
#endif  // DEVTOOL_TESTING_MOCK_GLOBAL_DEVTOOL_PLATFORM_FACADE_MOCK_H_
