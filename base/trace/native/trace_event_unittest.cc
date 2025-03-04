// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/trace_event.h"

#include <unistd.h>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_controller.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace trace {

class TraceEventTest : public ::testing::Test {
 protected:
  TraceEventTest() = default;
  ~TraceEventTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TraceEventTest, TraceEventMacrosTest) {
  auto controller = TraceController::Instance();
  auto trace_config = std::make_shared<TraceConfig>();
  std::string file_path = "";
  trace_config->file_path = file_path;
  auto session_id = controller->StartTracing(trace_config);
  ASSERT_TRUE(session_id != -1);

  // normal trace event
  TRACE_EVENT("TraceTest", "TraceEventTest0");

  // trace event with a lambda
  TRACE_EVENT("TraceTest", nullptr, [](lynx::perfetto::EventContext ctx) {
    auto* debug = ctx.event()->add_debug_annotations();
    debug->set_name("TraceTest");
    debug->set_string_value("YES");
  });

  // trace event with arbitrary number of debug annotations.
  TRACE_EVENT("TraceTest", "TraceEventTest1", "testKey1", "value1");
  TRACE_EVENT("TraceTest", "TraceEventTest2", "testKey1", "value1", "testKey2",
              "value2");
  TRACE_EVENT("TraceTest", "TraceEventTest3", "testKey1", 1234);

  // trace event with arbitrary number of debug annotations and a lambda.
  TRACE_EVENT("TraceTest", "TraceEventTest4", "testKey1", "value1",
              [](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("testKey2");
                debug->set_int_value(1234);
              });

  // Associate two trace events.
  uint64_t flow_id = TRACE_FLOW_ID();
  TRACE_EVENT("TraceTest", "TraceEventTest5-0",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(flow_id);
                ctx.event()->add_debug_annotations("startTimestamp", "1234");
              });
  TRACE_EVENT("TraceTest", "TraceEventTest5-1",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(flow_id);
                ctx.event()->add_debug_annotations("startTimestamp", "2345");
              });
  TRACE_EVENT_INSTANT("TraceTest", "TraceEventInstantTest0",
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_flow_ids(flow_id);
                      });

  // normal trace instant.
  TRACE_EVENT_INSTANT("TraceTest", "TraceEventInstantTest0");

  // trace instant with a lambda.
  TRACE_EVENT_INSTANT("TraceTest", "TraceEventInstantTest1",
                      [](lynx::perfetto::EventContext ctx) {
                        auto* debug = ctx.event()->add_debug_annotations();
                        debug->set_name("testKey2");
                        debug->set_int_value(1234);
                      });

  // trace instant with arbitrary number of debug annotations.
  TRACE_EVENT_INSTANT("TraceTest", "TraceEventInstantTest2", "testKey1",
                      "value1");

  // trace instant with arbitrary number of debug annotations and a lambda.
  TRACE_EVENT_INSTANT("TraceTest", "TraceEventInstantTest3", "testKey1",
                      "value1", [](lynx::perfetto::EventContext ctx) {
                        auto* debug = ctx.event()->add_debug_annotations();
                        debug->set_name("testKey2");
                        debug->set_int_value(1234);
                      });
  // trace counter
  TRACE_COUNTER("TraceTest", lynx::perfetto::CounterTrack("counter_tracker"),
                4);

  auto result = controller->StopTracing(session_id);
  ASSERT_TRUE(result);
  printf("TraceEventTest finish!\n");
}

}  // namespace trace
}  // namespace lynx
