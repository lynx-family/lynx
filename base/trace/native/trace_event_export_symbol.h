// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_EVENT_EXPORT_SYMBOL_H_
#define BASE_TRACE_NATIVE_TRACE_EVENT_EXPORT_SYMBOL_H_

#include <cstdint>

#include "base/trace/native/trace_export.h"

extern "C" {

TRACE_EXPORT void TraceEventBeginEx(const char *category,
                                    const char *event_name, int64_t trace_id,
                                    const char *arg1_name, const char *arg1_val,
                                    const char *arg2_name,
                                    const char *arg2_val);

TRACE_EXPORT void TraceEventEndEx(const char *category, const char *event_name,
                                  int64_t trace_id);

TRACE_EXPORT void TraceCounterEx(const char *category, const char *name,
                                 uint64_t counter, bool incremental);
}
#endif  // BASE_TRACE_NATIVE_TRACE_EVENT_EXPORT_SYMBOL_H_
