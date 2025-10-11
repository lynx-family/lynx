// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseTraceService.h>
#include "base/include/base_trace/trace_event_utils.h"
#include "base/trace/native/trace_event.h"

namespace lynx {
namespace base {
namespace trace {
void trace_backend(const char *category, const char *name, BaseTraceEventType phase) {
  switch (phase) {
    case BaseTraceEventType::TYPE_SLICE_BEGIN:
      TRACE_EVENT_BEGIN(category, name);
      break;
    case BaseTraceEventType::TYPE_SLICE_END:
      TRACE_EVENT_END(category);
      break;
    case BaseTraceEventType::TYPE_INSTANT:
      TRACE_EVENT_INSTANT(category, name);
      break;
    default:
      break;
  }
}
}  // namespace trace
}  // namespace base
}  // namespace lynx

@LynxServiceRegister(LynxBaseTraceService, LynxServiceTraceProtocol)
    @implementation LynxBaseTraceService

- (void *)getDefaultTraceFunction {
  return (void *)lynx::base::trace::trace_backend;
}

@end
