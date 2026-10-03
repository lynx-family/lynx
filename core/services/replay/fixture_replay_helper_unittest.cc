// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_set>

#include "core/services/replay/fixture_common.h"
#include "core/services/replay/fixture_js_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include <direct.h>

#include <random>
#else
#include <unistd.h>
#endif

namespace lynx {
namespace tasm {
namespace replay {
namespace {

bool CreateDirectory(const std::string& path) {
#if defined(OS_WIN)
  return _mkdir(path.c_str()) == 0;
#else
  return mkdir(path.c_str(), 0700) == 0;
#endif
}

using runtime::js::Object;
using runtime::js::Scope;

// Resource tests exercise the shared helpers without an evaluator/context.
class FixtureReplayHelperTest : public ::testing::Test {
 protected:
  void SetUp() override {
#if defined(OS_WIN)
    // Windows has no mkdtemp; mkdir still creates the directory exclusively.
    std::random_device random;
    for (int attempt = 0; attempt < 16; ++attempt) {
      auto candidate = ::testing::TempDir() + "lynx_fixture_replay_helper_" +
                       std::to_string(random());
      if (CreateDirectory(candidate)) {
        fixture_directory_ = candidate;
        break;
      }
    }
#else
    std::string directory_template =
        ::testing::TempDir() + "lynx_fixture_replay_helper_XXXXXX";
    char* directory = mkdtemp(directory_template.data());
    ASSERT_NE(directory, nullptr);
    fixture_directory_ = directory;
#endif
    ASSERT_FALSE(fixture_directory_.empty());
    ASSERT_TRUE(CreateDirectory(fixture_directory_ + "/assets"));
  }

  void TearDown() override {
    if (!fixture_directory_.empty()) {
      for (const auto& path : written_files_) {
        EXPECT_EQ(std::remove(path.c_str()), 0) << path;
      }
#if defined(OS_WIN)
      _rmdir((fixture_directory_ + "/assets").c_str());
      EXPECT_EQ(_rmdir(fixture_directory_.c_str()), 0);
#else
      rmdir((fixture_directory_ + "/assets").c_str());
      EXPECT_EQ(rmdir(fixture_directory_.c_str()), 0);
#endif
    }
  }

  void WriteAsset(const std::string& path, const std::string& content) {
    WriteFile(fixture_directory_ + "/assets/" + path, content);
  }

  void WriteFile(const std::string& path, const std::string& content) {
    std::ofstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    written_files_.insert(path);
    stream << content;
    ASSERT_TRUE(stream.good());
  }

  std::string fixture_directory_;
  std::unordered_set<std::string> written_files_;
};

TEST_F(FixtureReplayHelperTest, PreservesAssetTypesAndBothPathForms) {
  WriteAsset("valid.json", "{\"count\":3}");
  WriteAsset("invalid.json", "not json");
  WriteAsset("text.txt", "plain text");
  WriteAsset("empty.txt", "");
  auto fixture = CreateQuickJsFixtureRuntime();
  ASSERT_NE(fixture, nullptr);
  ScopedFixtureExecution execution(*fixture);
  auto& runtime = fixture->runtime();
  auto read_asset = CreateFixtureReadAssetFunction(runtime, fixture_directory_);
  const char* paths[] = {"valid.json", "assets/valid.json", "invalid.json",
                         "text.txt",   "empty.txt",         "missing.txt"};
  const char* expected[] = {"{\"count\":3}",  "{\"count\":3}", "\"not json\"",
                            "\"plain text\"", "null",          "null"};
  for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
    SCOPED_TRACE(paths[i]);
    const runtime::js::Value arg(
        runtime::js::String::createFromUtf8(runtime, paths[i]));
    auto result = read_asset.call(runtime, &arg, size_t{1});
    ASSERT_TRUE(result.has_value());
    if (i >= 4) {
      EXPECT_TRUE(result->isUndefined());
    }
    EXPECT_EQ(FixtureValueToJson(runtime, *result, "null"), expected[i]);
  }
}

TEST_F(FixtureReplayHelperTest, ReadsFilesWithinByteBudgets) {
  const auto directory = fixture_directory_;
  EXPECT_TRUE(ReadFixtureScript(directory).empty());
  EXPECT_TRUE(ReadFixtureAsset(directory, "data.bin").empty());
  const std::string data("a\0bc", 4);
  WriteFile(fixture_directory_ + "/fixture.js", data);
  WriteAsset("data.bin", data);
  EXPECT_EQ(ReadFixtureScript(directory, data.size()), data);
  EXPECT_EQ(ReadFixtureAsset(directory, "data.bin", data.size()), data);
  EXPECT_EQ(ReadFixtureAsset(directory, "assets/data.bin", data.size()), data);
  EXPECT_TRUE(ReadFixtureScript(directory, data.size() - 1).empty());
  EXPECT_TRUE(ReadFixtureAsset(directory, "data.bin", data.size() - 1).empty());
  EXPECT_TRUE(ReadFixtureScript(directory, 0).empty());
  EXPECT_TRUE(ReadFixtureAsset(directory, "data.bin", 0).empty());
  WriteFile(fixture_directory_ + "/fixture.js", "");
  WriteAsset("data.bin", "");
  EXPECT_TRUE(ReadFixtureScript(directory).empty());
  EXPECT_TRUE(ReadFixtureAsset(directory, "data.bin").empty());
}

TEST(FixtureReplayCommonTest, NormalizesDelayBoundaries) {
  EXPECT_EQ(NormalizeDelayMs(0), 0);
  EXPECT_EQ(NormalizeDelayMs(-1), 0);
  EXPECT_EQ(NormalizeDelayMs(std::numeric_limits<double>::quiet_NaN()), 0);
  EXPECT_EQ(NormalizeDelayMs(std::numeric_limits<double>::infinity()), 0);
  EXPECT_EQ(NormalizeDelayMs(-std::numeric_limits<double>::infinity()), 0);
  EXPECT_EQ(NormalizeDelayMs(0.5), 0);
  EXPECT_EQ(NormalizeDelayMs(12.75), 12);
  EXPECT_EQ(NormalizeDelayMs(
                static_cast<double>(std::numeric_limits<int64_t>::max())),
            std::numeric_limits<int64_t>::max());
  EXPECT_EQ(NormalizeDelayMs(std::numeric_limits<double>::max()),
            std::numeric_limits<int64_t>::max());
}

TEST_F(FixtureReplayHelperTest, RejectsUnsafeAssetPathForms) {
  WriteAsset("state.json", "{\"count\":3}");
  const std::string paths[] = {"",
                               "assets/",
                               "../assets/state.json",
                               "assets/../assets/state.json",
                               "./state.json",
                               "assets//state.json",
                               "state.json/",
                               "C:/state.json",
                               "file:state.json",
                               "assets\\state.json",
                               fixture_directory_ + "/assets/state.json"};
  for (const auto& path : paths) {
    SCOPED_TRACE(path);
    EXPECT_TRUE(ReadFixtureAsset(fixture_directory_, path).empty());
  }
}

void InstallContext(FixtureJsRuntime& fixture) {
  auto& runtime = fixture.runtime();
  Scope scope(runtime);
  runtime.global().setProperty(runtime, "ctx", Object(runtime));
}

TEST(FixtureReplayRuntimeTest, RetainsHandlersAcrossScriptExecutions) {
  auto fixture = CreateQuickJsFixtureRuntime();
  ASSERT_NE(fixture, nullptr);
  InstallContext(*fixture);
  ASSERT_TRUE(fixture
                  ->EvaluateScript(R"(
export default function(ctx) {
  ctx.count = 40;
  ctx.next = () => ++ctx.count;
}
)")
                  .has_value());
  ASSERT_TRUE(fixture->EvaluateScript("ctx.count += 1;").has_value());

  ScopedFixtureExecution execution(*fixture);
  auto& runtime = fixture->runtime();
  auto ctx = runtime.global().getProperty(runtime, "ctx");
  ASSERT_TRUE(ctx.has_value());
  auto next = ctx->getObject(runtime).getProperty(runtime, "next");
  ASSERT_TRUE(next.has_value());
  auto function = next->getObject(runtime).asFunction(runtime);
  ASSERT_TRUE(function.has_value());
  auto result = function->call(runtime, nullptr, 0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(FixtureValueToJson(runtime, *result, "null"), "42");
  EXPECT_FALSE(fixture->timed_out());
}

TEST(FixtureReplayRuntimeTest, KeepsInstancesIsolatedOnTheSameThread) {
  auto first = CreateQuickJsFixtureRuntime();
  ASSERT_NE(first, nullptr);
  InstallContext(*first);
  ASSERT_TRUE(first->EvaluateScript("ctx.value = 1;").has_value());
  {
    auto second = CreateQuickJsFixtureRuntime();
    ASSERT_NE(second, nullptr);
    InstallContext(*second);
    ASSERT_TRUE(second
                    ->EvaluateScript(R"(
if (ctx.value !== undefined) throw new Error('shared fixture state');
ctx.value = 2;
)")
                    .has_value());
  }
  EXPECT_TRUE(first
                  ->EvaluateScript(R"(
if (ctx.value !== 1) throw new Error('lost fixture state');
)")
                  .has_value());
}

TEST(FixtureReplayRuntimeTest,
     ResetsExecutionBudgetAfterTimeoutAndScriptError) {
  FixtureRuntimeLimits limits;
  limits.timeout_ms = 10;
  auto fixture = CreateQuickJsFixtureRuntime(limits);
  ASSERT_NE(fixture, nullptr);
  InstallContext(*fixture);
  {
    ScopedFixtureExecution outer_execution(*fixture);
    EXPECT_FALSE(
        fixture->EvaluateScript("for (let i = 0; i < 10000000; ++i) {};")
            .has_value());
    EXPECT_TRUE(fixture->timed_out());
    // A nested scope must not clear a timeout or give the operation a new
    // budget, even if the next piece of JavaScript is short.
    (void)fixture->EvaluateScript("ctx.value = 1;");
    EXPECT_TRUE(fixture->timed_out());
  }
  EXPECT_TRUE(fixture->EvaluateScript("ctx.value = 2;").has_value());
  EXPECT_FALSE(fixture->timed_out());
  EXPECT_FALSE(
      fixture->EvaluateScript("throw new Error('fixture error');").has_value());
  EXPECT_FALSE(fixture->timed_out());
  EXPECT_TRUE(fixture->EvaluateScript("ctx.value = 3;").has_value());
}

TEST(FixtureReplayRuntimeTest, BoundsHandlerCallsAndResultSerialization) {
  FixtureRuntimeLimits limits;
  limits.timeout_ms = 10;
  auto fixture = CreateQuickJsFixtureRuntime(limits);
  ASSERT_NE(fixture, nullptr);
  InstallContext(*fixture);
  ASSERT_TRUE(fixture
                  ->EvaluateScript(R"(
ctx.handler = () => ({
  toJSON() {
    for (let i = 0; i < 10000000; ++i) {}
    return 1;
  }
});
)")
                  .has_value());

  ScopedFixtureExecution execution(*fixture);
  auto& runtime = fixture->runtime();
  auto ctx = runtime.global().getProperty(runtime, "ctx");
  ASSERT_TRUE(ctx.has_value());
  auto handler = ctx->getObject(runtime).getProperty(runtime, "handler");
  ASSERT_TRUE(handler.has_value());
  auto function = handler->getObject(runtime).asFunction(runtime);
  ASSERT_TRUE(function.has_value());
  auto result = function->call(runtime, nullptr, 0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(FixtureValueToJson(runtime, *result, "null"), "null");
  EXPECT_TRUE(fixture->timed_out());
}

TEST(FixtureReplayRuntimeTest, CountsInstalledBindingsBeforeScriptCompilation) {
  FixtureRuntimeLimits limits;
  limits.memory_limit_bytes = 1024 * 1024;
  auto fixture = CreateQuickJsFixtureRuntime(limits);
  ASSERT_NE(fixture, nullptr);
  {
    auto& runtime = fixture->runtime();
    Scope scope(runtime);
    runtime.global().setProperty(
        runtime, "retained",
        runtime::js::String::createFromUtf8(runtime,
                                            std::string(2 * 1024 * 1024, 'x')));
  }
  EXPECT_FALSE(fixture->EvaluateScript("return 1;").has_value());
}

TEST(FixtureReplayRuntimeTest, RejectsDisabledLimitsAndAcceptsLongDeadlines) {
  FixtureRuntimeLimits limits;
  limits.timeout_ms = 0;
  EXPECT_EQ(CreateQuickJsFixtureRuntime(limits), nullptr);
  limits.timeout_ms = -1;
  EXPECT_EQ(CreateQuickJsFixtureRuntime(limits), nullptr);
  limits.timeout_ms = kDefaultFixtureTimeoutMs;
  limits.memory_limit_bytes = 0;
  EXPECT_EQ(CreateQuickJsFixtureRuntime(limits), nullptr);

  limits.memory_limit_bytes = 1;
  EXPECT_EQ(CreateQuickJsFixtureRuntime(limits), nullptr);

  limits = FixtureRuntimeLimits{};
  limits.timeout_ms = std::numeric_limits<int64_t>::max();
  auto fixture = CreateQuickJsFixtureRuntime(limits);
  ASSERT_NE(fixture, nullptr);
  InstallContext(*fixture);
  EXPECT_TRUE(fixture->EvaluateScript("for (let i = 0; i < 10000; ++i) {};")
                  .has_value());
  EXPECT_FALSE(fixture->timed_out());
}

}  // namespace
}  // namespace replay
}  // namespace tasm
}  // namespace lynx
