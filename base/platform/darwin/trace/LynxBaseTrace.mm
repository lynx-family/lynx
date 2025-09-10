// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxBase/LynxBaseService.h>
#import <LynxBase/LynxBaseServiceTraceProtocol.h>
#import <LynxBase/LynxBaseTrace.h>

#include "base/include/base_trace/trace_event_utils.h"

void InitLynxBaseTrace(void) {
  static id<LynxBaseServiceTraceProtocol> trace_service =
      LynxBaseService(LynxBaseServiceTraceProtocol);
  if (!trace_service) {
    return;
  }
  lynx::base::trace::SetTraceBackend(
      (lynx::base::trace::trace_backend_ptr)[trace_service getDefaultTraceFunction]);
}
