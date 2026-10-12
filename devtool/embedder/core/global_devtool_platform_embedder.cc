// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/global_devtool_platform_embedder.h"

#include <utility>

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "base/trace/native/trace_controller.h"
#endif
#include "core/renderer/tasm/config.h"
#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

namespace lynx {
namespace devtool {

class GlobalDevtoolPlatformCommon
    : public lynx::devtool::GlobalDevToolPlatformFacade {
 public:
  void StartMemoryTracing() override {
    GlobalDevtoolPlatformEmbedder::StartMemoryTracing();
  }

  void StopMemoryTracing() override {
    GlobalDevtoolPlatformEmbedder::StopMemoryTracing();
  }

  void GetAllMemoryUsage(
      int64_t timeout_ms,
      GlobalDevToolPlatformFacade::MemoryUsageCallback callback) override {
    GlobalDevtoolPlatformEmbedder::GetAllMemoryUsage(timeout_ms,
                                                     std::move(callback));
  }

  lynx::trace::TraceController* GetTraceController() override {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
    return lynx::trace::GetTraceControllerInstance();
#endif
    return nullptr;
  }

  lynx::trace::TracePlugin* GetFPSTracePlugin() override { return nullptr; }

  lynx::trace::TracePlugin* GetFrameViewTracePlugin() override {
    return nullptr;
  }

  lynx::trace::TracePlugin* GetInstanceTracePlugin() override {
    return nullptr;
  }

  lynx::trace::TracePlugin* GetMemoryTracePlugin() override { return nullptr; }

  std::string GetLynxVersion() override {
    return lynx::tasm::Config::GetCurrentLynxVersion();
  }
};

void GlobalDevtoolPlatformEmbedder::StartMemoryTracing() {}

void GlobalDevtoolPlatformEmbedder::StopMemoryTracing() {}

GlobalDevToolPlatformFacade& GlobalDevToolPlatformFacade::GetInstance() {
  static GlobalDevtoolPlatformCommon instance;
  return instance;
}
}  // namespace devtool

}  // namespace lynx
