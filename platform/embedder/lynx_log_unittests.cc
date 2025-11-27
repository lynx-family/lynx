// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <mutex>
#include <string>
#include <vector>

#include "base/include/log/logging.h"
#include "lynx/platform/embedder/public/capi/lynx_log_capi.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace {

struct TestLogSink {
  static std::mutex mutex;
  static std::vector<std::string> messages;

  static void CaptureLog(lynx::base::logging::LogMessage* msg,
                         const char* tag) {
    std::lock_guard<std::mutex> lock(mutex);
    messages.push_back(msg->stream().str());
  }

  static void Reset() {
    std::lock_guard<std::mutex> lock(mutex);
    messages.clear();
  }
};

// Define static members
std::mutex TestLogSink::mutex;
std::vector<std::string> TestLogSink::messages;

class LynxLogCAPITest : public ::testing::Test {
 protected:
  void SetUp() override {
    TestLogSink::Reset();
    // Hook into the logging system.
    lynx::base::logging::InitLynxLogging(nullptr, &TestLogSink::CaptureLog,
                                         true);
    lynx::base::logging::SetMinLogLevel(lynx::base::logging::LOG_VERBOSE);
  }

  void TearDown() override {
    // Restore the default logger to avoid affecting other tests.
    lynx::base::logging::InitLynxLogging(nullptr, nullptr, false);
  }
};

}  // namespace

TEST_F(LynxLogCAPITest, LogShortMessageWithMacro) {
  LYNX_CAPI_LOG(LYNX_LOG_INFO, "TestTag", "Hello, %s!", "World");

  ASSERT_EQ(TestLogSink::messages.size(), 1);
  const std::string& output = TestLogSink::messages[0];

  EXPECT_NE(output.find("[TestTag:"), std::string::npos);
  EXPECT_NE(output.find("lynx_log_unittests.cc"), std::string::npos);
  EXPECT_NE(output.find("] Hello, World!"), std::string::npos);
}

TEST_F(LynxLogCAPITest, LogLongMessageWithMacro) {
  std::string long_str(300, 'A');
  LYNX_CAPI_LOG(LYNX_LOG_WARNING, "LongTag", "Message: %s", long_str.c_str());

  ASSERT_EQ(TestLogSink::messages.size(), 1);
  const std::string& output = TestLogSink::messages[0];

  EXPECT_NE(output.find("[LongTag:"), std::string::npos);
  EXPECT_NE(output.find("lynx_log_unittests.cc"), std::string::npos);
  EXPECT_NE(output.find("] Message: "), std::string::npos);
  EXPECT_NE(output.find(long_str), std::string::npos);
}

TEST_F(LynxLogCAPITest, LogWithNoFormatArgsWithMacro) {
  LYNX_CAPI_LOG(LYNX_LOG_ERROR, "NoArgs", "This is a simple message.");

  ASSERT_EQ(TestLogSink::messages.size(), 1);
  const std::string& output = TestLogSink::messages[0];

  EXPECT_NE(output.find("[NoArgs:"), std::string::npos);
  EXPECT_NE(output.find("lynx_log_unittests.cc"), std::string::npos);
  EXPECT_NE(output.find("] This is a simple message."), std::string::npos);
}
