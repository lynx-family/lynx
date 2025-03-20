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

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_EVENT_H_
#define BASE_TRACE_NATIVE_TRACE_EVENT_H_

#include <utility>

// ================
// Quickstart guide
// ================
//
//   To add track events to your application, you can directly record
//   events with the TRACE_EVENT macros:
//
//       #include "trace_event.h"
//
//       int main() {
//
//         // A basic track event with just a name.
//         TRACE_EVENT("category", "MyEvent");
//
//         // A track event with (up to two) debug annotations.
//         TRACE_EVENT("category", "MyEvent", "parameter", 42);
//
//         // A track event with a strongly typed parameter.
//         TRACE_EVENT("category", "MyEvent", [](perfetto::EventContext ctx) {
//           ctx.event()->set_foo(42);
//           ctx.event()->set_bar(.5f);
//         });
//
//         // link two (or more) events (slices or instants), to mark them as
//         // related.
//         uint64_t flow_id = TRACE_FLOW_ID();
//         TRACE_EVENT("category", "MyEvent",
//                             [&](lynx::perfetto::EventContext ctx) {
//                               ctx.event()->add_flow_ids(flow_id);
//                             });
//         TRACE_EVENT("category", "OtherEvent",
//                             [&](lynx::perfetto::EventContext ctx) {
//                               ctx.event()->add_flow_ids(flow_id);
//                             });
//         TRACE_EVENT_INSTANT("category", "OtherEventInstant",
//                             [&](lynx::perfetto::EventContext ctx) {
//                               ctx.event()->add_flow_ids(flow_id);
//                             });
//
//         // A basic track event instant with just a name.
//         TRACE_EVENT_INSTANT("category", "MyEvent");
//
//         // A track event instant with (up to two) debug annotations.
//         TRACE_EVENT_INSTANT("category", "MyEvent", "parameter", 42);
//
//         // A track event instant with a strongly typed parameter.
//         TRACE_EVENT_INSTANT("category", "MyEvent", [](perfetto::EventContext
//         ctx) {
//           ctx.event()->set_foo(42);
//           ctx.event()->set_bar(.5f);
//         });
//
//         // A basic trace counter.
//         TRACE_COUNTER("category",
//         lynx::perfetto::CounterTrack("counter_tracker"), 4);
//
//       }
//
//  Note that track events must be nested consistently, i.e., the following is
//  not allowed:
//
//    TRACE_EVENT_BEGIN("a", "bar", ...);
//    TRACE_EVENT_BEGIN("b", "foo", ...);
//    TRACE_EVENT_END("a");  // "foo" must be closed before "bar".
//    TRACE_EVENT_END("b");
//

#if ENABLE_TRACE_PERFETTO  // ENABLE_TRACE_PERFETTO

#include "base/trace/native/trace_event_perfetto.h"

#elif ENABLE_TRACE_SYSTRACE  // ENABLE_TRACE_SYSTRACE

#include "base/trace/native/trace_event_systrace.h"

#else  // DISABLE_TRACE

#include "base/trace/native/trace_event_mock.h"

#endif

#if defined(__GNUC__) || defined(__clang__)
#define CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define CURRENT_FUNCTION __FUNCSIG__
#else
#define CURRENT_FUNCTION "(unknown)"
#endif

#define TRACE_EVENT_FUNC_NAME(category, ...) \
  TRACE_EVENT(category, CURRENT_FUNCTION, ##__VA_ARGS__)

#endif  // BASE_TRACE_NATIVE_TRACE_EVENT_H_
