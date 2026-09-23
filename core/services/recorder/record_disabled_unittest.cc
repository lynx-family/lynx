// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This source has its own recorder-disabled GN configuration.
#include "core/services/recorder/record.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm::recorder {
namespace {
class RecordMacro : public ::testing::Test {
 protected:
  void SetUp() override {
    min_level_ = base::logging::GetMinLogLevel();
    info_level_ = base::logging::GetInfoLogLevel();
    base::logging::SetMinLogLevel(base::logging::LOG_INFO);
  }
  void TearDown() override {
    base::logging::SetMinLogLevel(min_level_);
    base::logging::detail::g_info_log_level = info_level_;
  }
  int min_level_;
  int info_level_;
};

TEST_F(RecordMacro, FilteredArgumentsAndConditionsAreNotEvaluated) {
  int condition = 0, context = 0, arguments = 0;
  auto get_context = [&] {
    ++context;
    return base::LogContext{};
  };
  RECORD_OPTIONAL(++condition, Remove, get_context(), ++arguments);
  RECORD(Remove, get_context(), ++arguments);
  if (arguments == 0)
    RECORD(Remove, get_context(), ++arguments);
  else
    ++arguments;
  EXPECT_EQ(condition, 0);
  EXPECT_EQ(context, 0);
  EXPECT_EQ(arguments, 0);
}

TEST_F(RecordMacro, FilteredPreludeDoesNotRunOrReturn) {
  int evaluated = 0;
  auto invoke = [&evaluated]() {
    // clang-format off
    RECORD_WITH_EARLY_RETURN(++evaluated; return 7;
                            , OnlyObserve, base::LogContext{});
    // clang-format on
    return 11;
  };
  EXPECT_EQ(invoke(), 11);
  EXPECT_EQ(evaluated, 0);
}

TEST_F(RecordMacro, ObservationWorksWithoutRecorderAndEvaluatesOnce) {
  base::logging::SetMinLogLevel(base::logging::detail::LOG_OBSERVE);
  int contexts = 0, arguments = 0, formatted = 0;
  struct Props {
    int* formatted;
    int GetLength() const { return ++*formatted; }
  };
  auto get_context = [&] {
    ++contexts;
    return base::LogContext{1, 2, 3};
  };
  auto get_props = [&] {
    ++arguments;
    return Props{&formatted};
  };
  RECORD(SetGlobalProps, get_context(), get_props(), 0);
  EXPECT_EQ(contexts, 1);
  EXPECT_EQ(arguments, 1);
  EXPECT_EQ(formatted, 1);
  RECORD_OPTIONAL(false, SetGlobalProps, get_context(), get_props(), 0);
  EXPECT_EQ(contexts, 1);
  EXPECT_EQ(arguments, 1);
  EXPECT_EQ(formatted, 1);
}

TEST_F(RecordMacro, ObservationPreludeReturnsFromCaller) {
  base::logging::SetMinLogLevel(base::logging::detail::LOG_OBSERVE);
  auto invoke = [](bool stop) {
    // clang-format off
    RECORD_WITH_EARLY_RETURN(
        int value = 7;
        if (stop) { return value; }, OnlyObserve, base::LogContext{}, value);
    // clang-format on
    return 11;
  };
  EXPECT_EQ(invoke(true), 7);
  EXPECT_EQ(invoke(false), 11);
}
}  // namespace
}  // namespace lynx::tasm::recorder
