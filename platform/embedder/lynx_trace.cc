// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/public/capi/lynx_trace_capi.h"

// Include the Lynx internal trace macros
#include "base/trace/native/trace_event.h"

LYNX_EXTERN_C void lynx_trace_section_begin(const char* category,
                                            const char* name) {
  TRACE_EVENT_BEGIN(category, name);
}

LYNX_EXTERN_C void lynx_trace_section_end(const char* category,
                                          const char* name) {
  TRACE_EVENT_END(category);
}

LYNX_EXTERN_C void lynx_trace_instant(const char* category, const char* name) {
  TRACE_EVENT_INSTANT(category, name);
}

LYNX_EXTERN_C void lynx_trace_counter(const char* category, const char* name,
                                      uint64_t value, bool incremental) {
#if ENABLE_TRACE_PERFETTO
  // For Perfetto, we can construct a CounterTrack with the incremental flag.
  TRACE_COUNTER(category,
                lynx::perfetto::CounterTrack(name).set_incremental(incremental),
                value);
#else
  // For other backends (or when tracing is disabled), TRACE_COUNTER is a no-op
  // or doesn't support this. We call the macro with the name only.
  TRACE_COUNTER(category, name, value);
#endif
}
