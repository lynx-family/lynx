// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/tracing/platform/fps_trace_plugin_darwin.h"
#import <LynxDevtool/LynxFPSTrace.h>

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
namespace lynx {
namespace trace {

void FPSTracePluginDarwin::DispatchBegin() { [[LynxFPSTrace shareInstance] startFPSTrace]; }

void FPSTracePluginDarwin::DispatchEnd() { [[LynxFPSTrace shareInstance] stopFPSTrace]; }

std::string FPSTracePluginDarwin::Name() { return "FPS"; }

}  // namespace trace
}  // namespace lynx
#endif
