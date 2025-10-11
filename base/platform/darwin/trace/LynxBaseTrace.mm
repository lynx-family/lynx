// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxBase/LynxBaseTrace.h>
#import <LynxServiceAPI/LynxServiceTraceProtocol.h>
#import <LynxServiceAPI/ServiceAPI.h>

#include "base/include/base_trace/trace_event_utils.h"

void InitLynxBaseTrace(void) {
  static id<LynxServiceTraceProtocol> trace_service = LynxService(LynxServiceTraceProtocol);
  if (!trace_service) {
    return;
  }
  lynx::base::trace::SetTraceBackend(
      (lynx::base::trace::trace_backend_ptr)[trace_service getDefaultTraceFunction]);
}
