// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This source has its own recorder-disabled GN configuration.
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/recorder/record.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm::recorder {
namespace {
struct Props {
  int* formatted;
  int GetLength() const { return ++*formatted; }
  friend std::ostream& operator<<(std::ostream& stream, const Props& props) {
    ++*props.formatted;
    return stream << "props";
  }
};

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

TEST_F(RecordMacro, PayloadRequiresLoggingAndDevToolSwitches) {
  using namespace base::logging;
  auto& env = LynxEnv::GetInstance();
  const bool previous = env.GetBoolEnv("enable_devtool", false);
  env.SetBoolLocalEnv("enable_devtool", true);
  DevToolLifecycle::GetInstance().SyncStateFromPlatform(
      DevToolState::CONNECTED);
  SetRecordPayloadCallback([](const base::LogContext&, const char*,
                              std::string payload) -> std::string {
    EXPECT_EQ(payload, "props");
    return "test-id";
  });
  SetObservationPayloadEnabled(true);
  const base::LogContext context{123, 2, 3};
  int formatted = 0;
  Props props{&formatted};
  EXPECT_TRUE(ObservePayload(context, "test", "data", props).empty());
  SetMinLogLevel(detail::LOG_OBSERVE);
  SetObservationPayloadEnabled(false);
  EXPECT_TRUE(ObservePayload(context, "test", "data", props).empty());
  SetObservationPayloadEnabled(true);
  env.SetBoolLocalEnv("enable_devtool", false);
  EXPECT_TRUE(ObservePayload(context, "test", "data", props).empty());
  EXPECT_EQ(formatted, 0);
  env.SetBoolLocalEnv("enable_devtool", true);
  EXPECT_EQ(ObservePayload(context, "test", "data", props),
            " dataPayloadId:test-id");
  EXPECT_EQ(formatted, 1);
  SetRecordPayloadCallback(nullptr);
  EXPECT_TRUE(ObservePayload(context, "test", "data", props).empty());
  EXPECT_EQ(formatted, 1);
  SetObservationPayloadEnabled(false);
  env.SetBoolLocalEnv("enable_devtool", previous);
  DevToolLifecycle::GetInstance().SyncStateFromPlatform(
      DevToolState::UNAVAILABLE);
}

TEST_F(RecordMacro, UpdateMetaDataSendsGlobalPropsWithoutTemplateData) {
  auto& env = LynxEnv::GetInstance();
  const bool previous = env.GetBoolEnv("enable_devtool", false);
  env.SetBoolLocalEnv("enable_devtool", true);
  DevToolLifecycle::GetInstance().SyncStateFromPlatform(
      DevToolState::CONNECTED);
  SetRecordPayloadCallback([](const base::LogContext&, const char*,
                              std::string payload) -> std::string {
    EXPECT_EQ(payload, "props");
    return "test-id";
  });
  SetObservationPayloadEnabled(true);
  base::logging::SetMinLogLevel(base::logging::detail::LOG_OBSERVE);

  int data_formatted = 0, props_formatted = 0;
  Props data{&data_formatted}, global_props{&props_formatted};
  const Props* no_data = nullptr;
  RECORD(UpdateMetaData, base::LogContext{}, no_data, global_props, 0);
  EXPECT_EQ(data_formatted, 0);
  EXPECT_EQ(props_formatted, 1);
  RECORD(UpdateMetaData, base::LogContext{}, &data, global_props, 0);
  EXPECT_EQ(data_formatted, 1);
  EXPECT_EQ(props_formatted, 2);

  SetRecordPayloadCallback(nullptr);
  SetObservationPayloadEnabled(false);
  env.SetBoolLocalEnv("enable_devtool", previous);
  DevToolLifecycle::GetInstance().SyncStateFromPlatform(
      DevToolState::UNAVAILABLE);
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
