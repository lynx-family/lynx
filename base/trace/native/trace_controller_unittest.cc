// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/trace_controller.h"

#include <chrono>
#include <filesystem>
#include <string>

#include "base/include/file_utils.h"
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

class TraceDirectoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    previous_directory_ = base::GetApplicationDataDirectory();
    directory_ =
        (std::filesystem::temp_directory_path() /
         ("lynx-trace-" +
          std::to_string(
              std::chrono::steady_clock::now().time_since_epoch().count())))
            .u8string();
    std::filesystem::create_directory(std::filesystem::u8path(directory_));
    base::SetApplicationDataDirectory(directory_);
  }

  void TearDown() override {
    base::SetApplicationDataDirectory(previous_directory_);
    std::filesystem::remove_all(std::filesystem::u8path(directory_));
  }

  void ExpectTraceFile(const std::string& path) {
    auto* file = base::OpenFile(path, "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_NE(std::fgetc(file), EOF);
    std::fclose(file);
  }

  std::string previous_directory_;
  std::string directory_;
};

TEST_F(TraceDirectoryTest, DirectoryAvailableWithoutDelegate) {
  TraceController controller;
  EXPECT_EQ(controller.GetDelegate(), nullptr);
  EXPECT_EQ(controller.GenerateTracingFileDir(), directory_ + "/trace");
  const auto override_directory = directory_ + "/override";
  std::filesystem::create_directory(
      std::filesystem::u8path(override_directory));
  base::SetApplicationDataDirectory(override_directory);
  EXPECT_EQ(controller.GenerateTracingFileDir(), override_directory + "/trace");
}

#if ENABLE_TRACE_PERFETTO
TEST_F(TraceDirectoryTest, DefaultAndExplicitTracePaths) {
  auto* controller = TraceController::Instance();
  auto config = std::make_shared<TraceConfig>();
  auto session = controller->StartTracing(config);
  ASSERT_NE(session, -1);
  const auto default_path = config->file_path;
  EXPECT_EQ(default_path.find(directory_ + "/trace/lynx-profile-trace-"), 0u);
  base::SetApplicationDataDirectory(directory_ + "/unused");
  EXPECT_EQ(config->file_path, default_path);
  EXPECT_TRUE(controller->StopTracing(session));
  ExpectTraceFile(default_path);

  config = std::make_shared<TraceConfig>();
  config->file_path = directory_ + u8"/\u663e\u5f0f trace \U0001f680";
  session = controller->StartTracing(config);
  ASSERT_NE(session, -1);
  EXPECT_EQ(config->file_path, directory_ + u8"/\u663e\u5f0f trace \U0001f680");
  EXPECT_TRUE(controller->StopTracing(session));
  ExpectTraceFile(config->file_path);
}

TEST_F(TraceDirectoryTest, StartupConfigFollowsEffectiveDirectory) {
  auto* controller = TraceController::Instance();
  const std::string config =
      "{\"value\":\"" + std::string(5000, 'x') + u8"\U0001f680\"}";
  controller->SetStartupTracingConfig(config);
  EXPECT_EQ(controller->GetStartupTracingConfig(), config);
  EXPECT_TRUE(std::filesystem::is_regular_file(
      std::filesystem::u8path(directory_ + "/trace/trace-config.json")));
  EXPECT_FALSE(std::filesystem::exists(
      std::filesystem::u8path(directory_ + "/trace-config.json")));

  const auto override_directory =
      directory_ + u8"/\u5e94\u7528 data \U0001f680";
  std::filesystem::create_directory(
      std::filesystem::u8path(override_directory));
  base::SetApplicationDataDirectory(override_directory + "/");
  EXPECT_TRUE(controller->GetStartupTracingConfig().empty());
  controller->SetStartupTracingConfig("{\"override\":true}");
  EXPECT_EQ(controller->GetStartupTracingConfig(), "{\"override\":true}");
  EXPECT_TRUE(std::filesystem::is_regular_file(std::filesystem::u8path(
      override_directory + "/trace/trace-config.json")));
  base::SetApplicationDataDirectory(directory_);
  EXPECT_EQ(controller->GetStartupTracingConfig(), config);
}

TEST_F(TraceDirectoryTest, ContinuousTraceAndOpenFailure) {
  auto* controller = TraceController::Instance();
  auto config = std::make_shared<TraceConfig>();
  config->record_mode = TraceConfig::RECORD_CONTINUOUSLY;
  config->file_path = directory_ + u8"/\u8fde\u7eed trace \U0001f680";
  auto session = controller->StartTracing(config);
  ASSERT_NE(session, -1);
  EXPECT_TRUE(controller->StopTracing(session));
  ExpectTraceFile(config->file_path);

  config = std::make_shared<TraceConfig>();
  config->record_mode = TraceConfig::RECORD_CONTINUOUSLY;
  config->file_path = directory_ + "/missing/trace";
  EXPECT_EQ(controller->StartTracing(config), -1);
  EXPECT_FALSE(controller->IsTracingStarted());
  EXPECT_EQ(controller->GetLastSessionTraceConfig(), nullptr);
}

TEST_F(TraceDirectoryTest, DefaultDirectoryCreationFailureCanRetry) {
  auto* controller = TraceController::Instance();
  const auto child = directory_ + "/trace";
  auto* file = base::OpenFile(child, "wb");
  ASSERT_NE(file, nullptr);
  std::fclose(file);
  auto config = std::make_shared<TraceConfig>();
  EXPECT_EQ(controller->StartTracing(config), -1);
  EXPECT_TRUE(config->file_path.empty());
  EXPECT_FALSE(controller->IsTracingStarted());
  EXPECT_EQ(controller->GetLastSessionTraceConfig(), nullptr);

  ASSERT_EQ(base::RemoveFile(child), 0);
  const auto session = controller->StartTracing(config);
  ASSERT_NE(session, -1);
  EXPECT_EQ(config->file_path.find(child + "/lynx-profile-trace-"), 0u);
  EXPECT_TRUE(controller->StopTracing(session));
  ExpectTraceFile(config->file_path);
}

#if defined(OS_ANDROID) || defined(OS_HARMONY)
TEST_F(TraceDirectoryTest, EmptyMobileDirectoryCanRetry) {
  auto* controller = TraceController::Instance();
  base::SetApplicationDataDirectory({});
  auto config = std::make_shared<TraceConfig>();
  EXPECT_EQ(controller->StartTracing(config), -1);
  EXPECT_TRUE(config->file_path.empty());
  EXPECT_EQ(controller->GetLastSessionTraceConfig(), nullptr);
  base::SetApplicationDataDirectory(directory_);
  const auto session = controller->StartTracing(config);
  ASSERT_NE(session, -1);
  EXPECT_TRUE(controller->StopTracing(session));
  ExpectTraceFile(config->file_path);
}
#endif
#endif

}  // namespace trace
}  // namespace lynx
