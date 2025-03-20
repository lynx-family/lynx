// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_EVENT_SYSTRACE_H_
#define BASE_TRACE_NATIVE_TRACE_EVENT_SYSTRACE_H_

#include <utility>

#include "base/trace/native/trace_event_utils_systrace.h"

// The DecayStringType method is used to avoid unnecessary instantiations of
// templates on string constants of different sizes, like char[10] etc.
template <typename T>
[[maybe_unused]] static T&& DecayStringType(T&& t) {
  return std::forward<T>(t);
}

[[maybe_unused]] static inline const char* DecayStringType(const char* t) {
  return t;
}

namespace lynx {
namespace base {

class ScopedTracer {
 public:
  template <typename EventNameType>
  inline ScopedTracer(const EventNameType& name) {
    lynx::trace::TraceEventBegin(name);
  }

  inline ~ScopedTracer() { lynx::trace::TraceEventEnd(); }
};

}  // namespace base
}  // namespace lynx

#define INTERNAL_TRACE_EVENT_UID3(a, b) trace_event_uid_##a##b
#define INTERNAL_TRACE_EVENT_UID2(a, b) INTERNAL_TRACE_EVENT_UID3(a, b)
#define INTERNAL_TRACE_EVENT_UID(name) INTERNAL_TRACE_EVENT_UID2(name, __LINE__)

#define TRACE_EVENT(category, name, ...)                     \
  lynx::base::ScopedTracer INTERNAL_TRACE_EVENT_UID(tracer)( \
      DecayStringType(name));

#define TRACE_EVENT_BEGIN(category, name, ...) \
  lynx::trace::TraceEventBegin(DecayStringType(name))

#define TRACE_EVENT_END(category, ...) lynx::trace::TraceEventEnd()

#define TRACE_EVENT_INSTANT(category, name, ...)
#define TRACE_EVENT_CATEGORY_ENABLED(category) true
#define TRACE_COUNTER(category, track, ...)
#define TRACE_FLOW_ID() 0

#endif  // BASE_TRACE_NATIVE_TRACE_EVENT_SYSTRACE_H_
