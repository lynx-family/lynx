// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FPS_TRACE_PLUGIN_DARWIN_H_
#define DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FPS_TRACE_PLUGIN_DARWIN_H_

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "base/trace/native/trace_controller.h"

namespace lynx {
namespace trace {

class FPSTracePluginDarwin : public TracePlugin {
 public:
  FPSTracePluginDarwin() = default;
  virtual ~FPSTracePluginDarwin() = default;
  virtual void DispatchBegin() override;
  virtual void DispatchEnd() override;
  virtual std::string Name() override;
};

}  // namespace trace
}  // namespace lynx
#endif
#endif  // DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_FPS_TRACE_PLUGIN_DARWIN_H_
