// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/skity_event_tracer_impl.h"

#define TRACE_EVENT_HIDE_MACROS
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/fml/posix_wrappers.h"
#include "skity/utils/trace_event.hpp"

namespace clay {

static void SkityTraceBegin(const char* category_group,
                            const char* section_name, int64_t trace_id,
                            const char* arg1_name, const char* arg1_val,
                            const char* arg2_name, const char* arg2_val) {
  if (arg1_name != nullptr && arg2_name != nullptr) {
    TRACE_EVENT_BEGIN(category_group, section_name, arg1_name,
                      arg1_val ? arg1_val : "", arg2_name,
                      arg2_val ? arg2_val : "");
  } else if (arg1_name != nullptr) {
    TRACE_EVENT_BEGIN(category_group, section_name, arg1_name,
                      arg1_val ? arg1_val : "");
  } else {
    TRACE_EVENT_BEGIN(category_group, section_name);
  }
}

static void SkityTraceEnd(const char* category_group, const char* section_name,
                          int64_t trace_id) {
  TRACE_EVENT_END(category_group);
}

static void SkityTraceCounter(const char* category, const char* name,
                              uint64_t counter, bool incremental) {
  if (name) {
    TRACE_COUNTER(category, name, counter);
  }
}

void InitSkityEventTracer() {
  skity::SkityTraceHandler handler;
  handler.begin_section = SkityTraceBegin;
  handler.end_section = SkityTraceEnd;
  handler.counter = SkityTraceCounter;
  skity::InjectTraceHandler(handler);
}

}  // namespace clay
