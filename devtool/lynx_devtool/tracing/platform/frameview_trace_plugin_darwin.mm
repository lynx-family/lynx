// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/tracing/platform/frameview_trace_plugin_darwin.h"
#import "LynxFrameViewTrace.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
namespace lynx {
namespace trace {

void FrameViewTracePluginDarwin::DispatchBegin() {
  [[LynxFrameViewTrace shareInstance] startFrameViewTrace];
}

void FrameViewTracePluginDarwin::DispatchEnd() {
  [[LynxFrameViewTrace shareInstance] stopFrameViewTrace];
}

std::string FrameViewTracePluginDarwin::Name() { return "FrameView"; }

}  // namespace trace
}  // namespace lynx
#endif
