// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_SRC_BASE_TRACE_TRACE_EVENT_H_
#define BASE_SRC_BASE_TRACE_TRACE_EVENT_H_

#include "base/include/base_trace/trace_event_utils.h"

#define BASE_INTERNAL_TRACE_EVENT_UID3(a, b) trace_event_uid_##a##b
#define BASE_INTERNAL_TRACE_EVENT_UID2(a, b) \
  BASE_INTERNAL_TRACE_EVENT_UID3(a, b)
#define BASE_INTERNAL_TRACE_EVENT_UID(name) \
  BASE_INTERNAL_TRACE_EVENT_UID2(name, __LINE__)

#define BASE_TRACE_EVENT(category, name)                              \
  struct BASE_INTERNAL_TRACE_EVENT_UID(ScopedEvent) {                 \
    struct BaseEventFinalizer {                                       \
      BaseEventFinalizer(...) {}                                      \
      ~BaseEventFinalizer() { BASE_TRACE_EVENT_END(category, name); } \
    } base_finalizer;                                                 \
  } BASE_INTERNAL_TRACE_EVENT_UID(scoped_event) {                     \
    [&]() {                                                           \
      BASE_TRACE_EVENT_BEGIN(category, name);                         \
      return 0;                                                       \
    }()                                                               \
  }

#define BASE_TRACE_EVENT_BEGIN(category, name) \
  lynx::base::trace::TraceEventBegin(category, name)
#define BASE_TRACE_EVENT_END(category, name) \
  lynx::base::trace::TraceEventEnd(category, name)

#endif  // BASE_SRC_BASE_TRACE_TRACE_EVENT_H_
