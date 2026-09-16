// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"

#include <cstdint>
#include <limits>
#include <string>

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

TEST_F(LogLevelTest, ClampsThresholdWithoutDisablingErrors) {
  SetMinLogLevel(LOG_FATAL);
  EXPECT_EQ(GetMinLogLevel(), LOG_ERROR);
  EXPECT_TRUE(LOG_IS_ON(ERROR));
  EXPECT_TRUE(LOG_IS_ON(FATAL));
  SetMinLogLevel(LOG_INFO);
  EXPECT_TRUE(LOG_IS_ON(ERROR));
  EXPECT_TRUE(LOG_IS_ON(INFO));
}

TEST_F(LogLevelTest, ClampsOutOfRangeThresholds) {
  SetMinLogLevel(-100);
  EXPECT_EQ(GetMinLogLevel(), LOG_VERBOSE);
  SetMinLogLevel(100);
  EXPECT_EQ(GetMinLogLevel(), LOG_ERROR);
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
