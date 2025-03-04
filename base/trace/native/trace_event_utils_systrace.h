// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_EVENT_UTILS_SYSTRACE_H_
#define BASE_TRACE_NATIVE_TRACE_EVENT_UTILS_SYSTRACE_H_

#include <dlfcn.h>

#include <string>

#include "base/include/log/logging.h"
#include "base/trace/native/internal_trace_category.h"
#include "base/trace/native/trace_export.h"
#include "base/trace/native/track_event_wrapper.h"

namespace lynx {
namespace trace {

using ATrace_beginSection_ptr = void *(*)(const char *section_name);
using ATrace_endSection_ptr = void *(*)(void);
using ATrace_beginAsyncSection_ptr = void *(*)(const char *section_name,
                                               int32_t cookie);
using ATrace_endAsyncSection_ptr = void *(*)(const char *section_name,
                                             int32_t cookie);

void InitSystraceBeginSection(ATrace_beginSection_ptr atrace_beginsection);
void InitSystraceEndSection(ATrace_endSection_ptr atrace_endsection);
void InitSystraceBeginAsynSection(
    ATrace_beginAsyncSection_ptr atrace_beginasyncsection);
void InitSystraceEndAsynSection(
    ATrace_endAsyncSection_ptr atrace_endasyncsection);

TRACE_EXPORT void TraceEventBegin(const char *name);
TRACE_EXPORT void TraceEventBegin(const std::string &name);
TRACE_EXPORT void TraceEventBegin(const char *name, uint64_t cookie);
TRACE_EXPORT void TraceEventBegin(const std::string &name, uint64_t cookie);
TRACE_EXPORT void TraceEventEnd();
TRACE_EXPORT void TraceEventEnd(const char *name, uint64_t cookie);

template <typename EventNameType, typename TrackType, typename... Arguments,
          typename TrackTypeCheck = typename std::enable_if_t<
              std::is_same_v<TrackType, lynx::perfetto::Track>>>
TRACE_EXPORT void TraceEventBegin(const EventNameType &name, TrackType track_id,
                                  Arguments &&...args) {
  const char *event_name = name == nullptr ? "" : name;
  TraceEventBegin(event_name, track_id);
}
template <typename EventNameType, typename... Arguments>
TRACE_EXPORT void TraceEventBegin(const EventNameType &name,
                                  Arguments &&...args) {
  const char *event_name = name == nullptr ? "" : name;
  TraceEventBegin(event_name);
}

template <typename TrackType, typename... Arguments,
          typename TrackTypeCheck = typename std::enable_if_t<
              std::is_same_v<TrackType, lynx::perfetto::Track>>>
TRACE_EXPORT void TraceEventEnd(TrackType track_id, Arguments &&...args) {
  TraceEventEnd("end", track_id);
}
template <typename... Arguments>
TRACE_EXPORT void TraceEventEnd(Arguments &&...args) {
  TraceEventEnd();
}

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_TRACE_EVENT_UTILS_SYSTRACE_H_
