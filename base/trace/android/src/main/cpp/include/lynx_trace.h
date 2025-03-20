// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_ANDROID_SRC_MAIN_CPP_INCLUDE_LYNX_TRACE_H_
#define BASE_TRACE_ANDROID_SRC_MAIN_CPP_INCLUDE_LYNX_TRACE_H_

#include "base/trace/native/trace_event.h"

// Please use `#include <lynxtrace/lynx_trace.h>` to use lynx trace public APIs.

// We provide the following trace event macro APIs:
// 1. TRACE_EVENT(category, name, ...): Records the duration from the start of
// the trace to the end of the scope.
// 2. TRACE_EVENT_BEGIN(category, name, ...): Marks the start point of a
// category trace.
// 3. TRACE_EVENT_END(category, ...): Marks the end point of a category trace.
// 4. TRACE_EVENT_INSTANT(category, name, ...): Instant event.
// 5. TRACE_COUNTER(category, track, ...): Counter event.
// 6. TRACE_FLOW_ID(): Gets a flow_id for linking trace events.

#endif  // BASE_TRACE_ANDROID_SRC_MAIN_CPP_INCLUDE_LYNX_TRACE_H_
