// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/trace_event_export_symbol.h"

#include <cstdint>

#include "base/trace/native/trace_event.h"

extern "C" {

static const int64_t kTraceEventSync = -1;
static const int64_t kTraceEventInstant = -2;  // not support now.
void TraceEventBeginEx(const char *category, const char *event_name,
                       int64_t trace_id, const char *arg1_name,
                       const char *arg1_val, const char *arg2_name,
                       const char *arg2_val) {
  if (trace_id == kTraceEventSync) {
    TRACE_EVENT_BEGIN(category, nullptr, [=](lynx::perfetto::EventContext ctx) {
      auto *event = ctx.event();
      event->set_name(event_name);
      if (arg1_name && arg1_val) {
        event->add_debug_annotations(arg1_name, arg1_val);
      }
      if (arg2_name && arg2_val) {
        event->add_debug_annotations(arg2_name, arg2_val);
      }
    });

  } else if (trace_id == kTraceEventInstant) {
    TRACE_EVENT_INSTANT(category, nullptr,
                        [=](lynx::perfetto::EventContext ctx) {
                          auto *event = ctx.event();
                          event->set_name(event_name);
                          if (arg1_name && arg1_val) {
                            event->add_debug_annotations(arg1_name, arg1_val);
                          }
                          if (arg2_name && arg2_val) {
                            event->add_debug_annotations(arg2_name, arg2_val);
                          }
                        });
  }
}

void TraceEventEndEx(const char *category, const char *event_name,
                     int64_t trace_id) {
  if (trace_id < 0) {  // sync
    TRACE_EVENT_END(category, [=](lynx::perfetto::EventContext ctx) {
      ctx.event()->set_name(event_name);
    });
  }
}

void TraceCounterEx(const char *category, const char *name, uint64_t counter,
                    bool incremental) {
  TRACE_COUNTER(
      category,
      lynx::perfetto::CounterTrack(name).set_category(category).set_incremental(
          incremental),
      counter);
}
}
