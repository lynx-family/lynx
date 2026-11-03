// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/trace_controller.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace trace {

namespace {
class MemoryTraceTestController : public TraceController {
 public:
  bool IsTracingStarted() override { return tracing_started; }
  std::shared_ptr<TraceConfig> GetLastSessionTraceConfig() override {
    return config;
  }

  bool tracing_started{false};
  std::shared_ptr<TraceConfig> config;
};
}  // namespace

class TraceControllerTest : public ::testing::Test {
 protected:
  TraceControllerTest() = default;
  ~TraceControllerTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TraceControllerTest, MemoryTraceConfigRequiresActiveMemoryTrace) {
  MemoryTraceTestController controller;
  for (bool started : {false, true}) {
    for (bool has_config : {false, true}) {
      for (bool memory_trace : {false, true}) {
        SCOPED_TRACE(::testing::Message()
                     << started << "/" << has_config << "/" << memory_trace);
        controller.tracing_started = started;
        controller.config =
            has_config ? std::make_shared<TraceConfig>() : nullptr;
        if (controller.config) {
          controller.config->enable_memory_trace = memory_trace;
        }
        EXPECT_EQ(controller.GetMemoryTraceConfig(),
                  started && has_config && memory_trace ? controller.config
                                                        : nullptr);
      }
    }
  }
}

TEST_F(TraceControllerTest, TraceControllerTotalTest) {
  auto controller = TraceController::Instance();
  static int const kDefaultBufferSize = 40960;
  int bufferSize = kDefaultBufferSize;
  std::string file_path = "";
  auto trace_config = std::make_shared<lynx::trace::TraceConfig>();
  trace_config->buffer_size = bufferSize;
  trace_config->file_path = file_path;
  trace_config->included_categories = {"*"};
  trace_config->excluded_categories = {"*"};
  auto session_id = controller->StartTracing(trace_config);
  ASSERT_TRUE(session_id != -1);

  controller->AddCompleteCallback(session_id,
                                  []() { printf("AddCompleteCallback\n"); });

  auto result = controller->StopTracing(session_id);
  ASSERT_TRUE(result);
  controller->RemoveCompleteCallbacks(session_id);
  printf("TraceControllerTest finish!\n");
}

}  // namespace trace
}  // namespace lynx
