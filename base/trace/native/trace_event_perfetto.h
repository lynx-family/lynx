// Copyright (C) 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_EVENT_PERFETTO_H_
#define BASE_TRACE_NATIVE_TRACE_EVENT_PERFETTO_H_

#include <utility>

#include "base/trace/native/trace_event_utils_perfetto.h"

// The DecayStringType method is used to avoid unnecessary instantiations of
// templates on string constants of different sizes, like char[10] etc.
template <typename T>
[[maybe_unused]] static T&& DecayStringType(T&& t) {
  return std::forward<T>(t);
}

[[maybe_unused]] static inline const char* DecayStringType(const char* t) {
  return t;
}

using TraceEvent = lynx::perfetto::TrackEvent;

// tools
#define INTERNAL_TRACE_EVENT_UID3(a, b) trace_event_uid_##a##b
#define INTERNAL_TRACE_EVENT_UID2(a, b) INTERNAL_TRACE_EVENT_UID3(a, b)
#define INTERNAL_TRACE_EVENT_UID(name) INTERNAL_TRACE_EVENT_UID2(name, __LINE__)

#define TRACE_EVENT(category, name, ...)                                      \
  struct INTERNAL_TRACE_EVENT_UID(ScopedEvent) {                              \
    struct EventFinalizer {                                                   \
      /* The parameter is an implementation detail. It allows the          */ \
      /* anonymous struct to use aggregate initialization to invoke the    */ \
      /* lambda (which emits the BEGIN event and returns an integer)       */ \
      /* with the proper reference capture for any                         */ \
      /* TrackEventArgumentFunction in |__VA_ARGS__|. This is required so  */ \
      /* that the scoped event is exactly ONE line and can't escape the    */ \
      /* scope if used in a single line if statement.                      */ \
      EventFinalizer(...) {}                                                  \
      ~EventFinalizer() { TRACE_EVENT_END(category); }                        \
    } finalizer;                                                              \
  } INTERNAL_TRACE_EVENT_UID(scoped_event) {                                  \
    [&]() {                                                                   \
      TRACE_EVENT_BEGIN(category, name, ##__VA_ARGS__);                       \
      return 0;                                                               \
    }()                                                                       \
  }

#define TRACE_EVENT_BEGIN(category, name, ...) \
  lynx::trace::TraceEventBegin(category, DecayStringType(name), ##__VA_ARGS__)
#define TRACE_EVENT_END(category, ...) \
  lynx::trace::TraceEventEnd(category, ##__VA_ARGS__)
#define TRACE_EVENT_INSTANT(category, name, ...) \
  lynx::trace::TraceEventInstant(category, DecayStringType(name), ##__VA_ARGS__)
#define TRACE_EVENT_CATEGORY_ENABLED(category) \
  lynx::trace::TraceEventCategoryEnabled(category)
#define TRACE_COUNTER(category, track, ...)                                \
  lynx::trace::TraceCounter(category, lynx::perfetto::CounterTrack(track), \
                            ##__VA_ARGS__)
#define TRACE_FLOW_ID() lynx::trace::GetFlowId()

#endif  // BASE_TRACE_NATIVE_TRACE_EVENT_PERFETTO_H_
