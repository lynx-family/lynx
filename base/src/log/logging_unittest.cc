// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "base/include/log/log_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::base::logging {
namespace {

class LogLevelTest : public ::testing::Test {
 protected:
  void SetUp() override { original_level_ = GetMinLogLevel(); }
  void TearDown() override { SetMinLogLevel(original_level_); }

 private:
  int original_level_;
};

TEST_F(LogLevelTest, RestoresMoreDetailedLogging) {
  SetMinLogLevel(LOG_INFO);
  EXPECT_TRUE(LOG_IS_ON(INFO));
  SetMinLogLevel(LOG_WARNING);
  EXPECT_FALSE(LOG_IS_ON(INFO));
  SetMinLogLevel(LOG_INFO);
  EXPECT_TRUE(LOG_IS_ON(INFO));
  SetMinLogLevel(LOG_DEBUG);
  EXPECT_TRUE(LOG_IS_ON(DEBUG));
  SetMinLogLevel(LOG_VERBOSE);
  EXPECT_TRUE(LOG_IS_ON(VERBOSE));
}

TEST_F(LogLevelTest, PreservesFatalThresholdAndCanRestoreIt) {
  SetMinLogLevel(LOG_FATAL);
  EXPECT_FALSE(LOG_IS_ON(ERROR));
  EXPECT_TRUE(LOG_IS_ON(FATAL));
  SetMinLogLevel(LOG_INFO);
  EXPECT_TRUE(LOG_IS_ON(ERROR));
  EXPECT_TRUE(LOG_IS_ON(INFO));
}

TEST_F(LogLevelTest, EvaluatesPayloadOnlyAfterRestoringThreshold) {
  int evaluations = 0;
  SetMinLogLevel(LOG_WARNING);
  BASE_LOG(INFO) << ++evaluations;
  EXPECT_EQ(evaluations, 0);
  SetMinLogLevel(LOG_INFO);
  BASE_LOG(INFO) << ++evaluations;
  EXPECT_EQ(evaluations, 1);
}

struct CapturedLog {
  LogSeverity severity;
  std::string text;
};

std::vector<CapturedLog>* captured_logs = nullptr;

void CaptureLog(LogMessage* message, const char*) {
  if (captured_logs) {
    captured_logs->push_back({message->severity(), message->stream().str()});
  }
}

class LogLevelDiagnosticTest : public LogLevelTest {
 protected:
  void SetUp() override {
    LogLevelTest::SetUp();
    InitLynxLogging(nullptr, CaptureLog, true);
    SetMinLogLevel(LOG_INFO);
    captured_logs = &logs_;
  }
  void TearDown() override {
    captured_logs = nullptr;
    LogLevelTest::TearDown();
  }
  std::vector<CapturedLog> logs_;
};

TEST_F(LogLevelDiagnosticTest, ReportsChangesWithWarningFloor) {
  struct Case {
    int threshold;
    int severity;
    const char* transition;
  };
  const Case cases[] = {
      {LOG_ERROR, LOG_ERROR, "INFO (2) -> ERROR (4)"},
      {LOG_WARNING, LOG_WARNING, "ERROR (4) -> WARNING (3)"},
      {LOG_INFO, LOG_WARNING, "WARNING (3) -> INFO (2)"},
      {LOG_DEBUG, LOG_WARNING, "INFO (2) -> DEBUG (1)"},
      {LOG_VERBOSE, LOG_WARNING, "DEBUG (1) -> VERBOSE (0)"},
  };
  for (const auto& test : cases) {
    logs_.clear();
    SetMinLogLevel(test.threshold);
    ASSERT_EQ(logs_.size(), 1u);
    EXPECT_EQ(logs_[0].severity, test.severity);
    EXPECT_NE(logs_[0].text.find(test.transition), std::string::npos);
    EXPECT_EQ(GetMinLogLevel(), test.threshold);
  }
}

TEST_F(LogLevelDiagnosticTest, UnchangedEffectiveLevelDoesNotLog) {
  SetMinLogLevel(LOG_INFO);
  EXPECT_TRUE(logs_.empty());
  SetMinLogLevel(LOG_FATAL);
  logs_.clear();
  SetMinLogLevel(LOG_FATAL + 100);
  EXPECT_TRUE(logs_.empty());
}

TEST_F(LogLevelDiagnosticTest, FatalThresholdDoesNotAbort) {
  SetMinLogLevel(LOG_FATAL + 100);
  EXPECT_EQ(GetMinLogLevel(), LOG_FATAL);
  ASSERT_EQ(logs_.size(), 1u);
  EXPECT_EQ(logs_[0].severity, LOG_ERROR);
  EXPECT_NE(logs_[0].text.find("INFO (2) -> FATAL (5)"), std::string::npos);

  logs_.clear();
  SetMinLogLevel(-1);
  EXPECT_EQ(GetMinLogLevel(), -1);
  ASSERT_EQ(logs_.size(), 1u);
  EXPECT_EQ(logs_[0].severity, LOG_WARNING);
  EXPECT_NE(logs_[0].text.find("FATAL (5) -> UNKNOWN (-1)"), std::string::npos);
}

TEST_F(LogLevelDiagnosticTest, OrdinaryFatalLogStillAborts) {
  EXPECT_DEATH_IF_SUPPORTED(
      {
        SetMinLogLevel(LOG_FATAL);
        InitLynxLogging(
            nullptr,
            [](LogMessage* message, const char*) {
              std::fputs(message->stream().str().c_str(), stderr);
              std::fflush(stderr);
            },
            true);
        BASE_LOG(FATAL) << "fatal payload";
      },
      "fatal payload");
}

TEST(LogContextTest, DefaultsToUnavailableEntities) {
  LogContext context;

  EXPECT_EQ(context.view_id, kUnavailableLynxEntityId);
  EXPECT_EQ(context.engine_id, kUnavailableLynxEntityId);
  EXPECT_EQ(context.runtime_id, kUnavailableLynxEntityId);
}

TEST(LogContextTest, SerializesExactTupleAndSnapshotsRuntime) {
  LogContext context{12, 34, 56};
  LogMessage message("logging_unittest.cc", 1, LOG_INFO);
  message.stream() << context << " load template";

  EXPECT_EQ(message.stream().str().substr(message.messageStart()),
            "[12,34,56] load template");

  context = {100, 200, 300};
  EXPECT_EQ(message.stream().str().substr(message.messageStart()),
            "[12,34,56] load template");
}

TEST(LogContextTest, SerializesUnavailableZeroAndMaximum) {
  LogContext context{kUnavailableLynxEntityId, 0,
                     std::numeric_limits<int32_t>::max()};
  LogMessage message("logging_unittest.cc", 1, LOG_WARNING);
  message.stream() << context;

  EXPECT_EQ(message.stream().str().substr(message.messageStart()),
            "[-1,0,2147483647]");
}

TEST(LogContextTest, OrdinaryNativeLogBehaviorIsUnchanged) {
  LogMessage message("logging_unittest.cc", 1, LOG_INFO);
  message.stream() << "ordinary payload";

  EXPECT_EQ(message.stream().str().substr(message.messageStart()),
            "ordinary payload");
}

}  // namespace
}  // namespace lynx::base::logging
