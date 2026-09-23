// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_evaluator.h"

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_set>

#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

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

class FixtureEvaluatorTest : public ::testing::Test {
 protected:
  void SetUp() override {
#if defined(OS_WIN)
    // Windows has no mkdtemp; mkdir still creates the directory exclusively.
    std::random_device random;
    for (int attempt = 0; attempt < 16; ++attempt) {
      auto candidate = ::testing::TempDir() + "lynx_fixture_evaluator_" +
                       std::to_string(random());
      if (CreateDirectory(candidate)) {
        fixture_directory_ = candidate;
        break;
      }
    }
#else
    std::string directory_template =
        ::testing::TempDir() + "lynx_fixture_evaluator_XXXXXX";
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

  void WriteFixture(const std::string& source) {
    WriteFile(fixture_directory_ + "/fixture.js", source);
  }

  void WriteAsset(const std::string& path, const std::string& content) {
    auto file_path = fixture_directory_ + "/assets/" + path;
    WriteFile(file_path, content);
  }

  std::string fixture_directory_;
  std::unordered_set<std::string> written_files_;

 private:
  void WriteFile(const std::string& path, const std::string& content) {
    std::ofstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    written_files_.insert(path);
    stream << content;
    ASSERT_TRUE(stream.good());
  }
};

TEST_F(FixtureEvaluatorTest, ReportsMissingFixtureScript) {
  auto result = EvaluateFixture(fixture_directory_);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "fixture.js is missing or empty");
}

TEST_F(FixtureEvaluatorTest, EvaluatesLifecycleActionsAndSharedData) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.setThreadStrategy({"id": 1});
  ctx.setGlobalProps({"theme": "dark"});
  ctx.loadTemplate("https://example.com/template.js",
                   "assets/template/template.bin", {"count": 2});
  ctx.after(25, () => ctx.dispatch("customAction", {"enabled": true}));
  ctx.sharedData("state", {"ready": true});
  ctx.register("ExampleModule", "method", () => null);
}
)");

  auto result = EvaluateFixture(fixture_directory_);

  ASSERT_TRUE(result.has_value()) << result.error();
  ASSERT_EQ(result.value().actions.size(), 4u);
  EXPECT_EQ(result.value().actions[0].function_name, "setThreadStrategy");
  EXPECT_EQ(result.value().actions[0].delay_ms, 0);
  EXPECT_EQ(result.value().actions[1].function_name, "setGlobalProps");
  EXPECT_EQ(result.value().actions[2].function_name, "loadTemplate");
  EXPECT_EQ(result.value().actions[3].function_name, "customAction");
  EXPECT_EQ(result.value().actions[3].delay_ms, 25);

  rapidjson::Document load_template_params;
  load_template_params.Parse(result.value().actions[2].params_json.c_str());
  ASSERT_TRUE(load_template_params.IsObject());
  ASSERT_TRUE(load_template_params.HasMember("url"));
  ASSERT_TRUE(load_template_params["url"].IsString());
  EXPECT_STREQ(load_template_params["url"].GetString(),
               "https://example.com/template.js");
  ASSERT_TRUE(load_template_params.HasMember("templateAsset"));
  ASSERT_TRUE(load_template_params["templateAsset"].IsString());
  EXPECT_STREQ(load_template_params["templateAsset"].GetString(),
               "assets/template/template.bin");
  ASSERT_TRUE(load_template_params.HasMember("templateData"));
  ASSERT_TRUE(load_template_params["templateData"].IsObject());
  ASSERT_TRUE(load_template_params["templateData"].HasMember("count"));
  ASSERT_TRUE(load_template_params["templateData"]["count"].IsInt());
  EXPECT_EQ(load_template_params["templateData"]["count"].GetInt(), 2);
  ASSERT_TRUE(load_template_params.HasMember("isCSR"));
  ASSERT_TRUE(load_template_params["isCSR"].IsBool());
  EXPECT_TRUE(load_template_params["isCSR"].GetBool());

  ASSERT_EQ(result.value().shared_data.size(), 1u);
  EXPECT_EQ(result.value().shared_data[0].key, "state");
  EXPECT_EQ(result.value().shared_data[0].value_json, "{\"ready\":true}");
}

TEST_F(FixtureEvaluatorTest, ReadsJsonAssetsAndRejectsUnsafePaths) {
  WriteAsset("state.json", "{\"count\":3}");
  WriteFixture(R"(
export default function(ctx) {
  ctx.setGlobalProps(ctx.readAsset("assets/state.json"));
  ctx.sharedData("unsafe", ctx.readAsset("../outside.json"));
}
)");

  auto result = EvaluateFixture(fixture_directory_);

  ASSERT_TRUE(result.has_value()) << result.error();
  ASSERT_EQ(result.value().actions.size(), 1u);
  EXPECT_EQ(result.value().actions[0].params_json,
            "{\"global_props\":{\"count\":3}}");
  ASSERT_EQ(result.value().shared_data.size(), 1u);
  EXPECT_EQ(result.value().shared_data[0].value_json, "null");
}

TEST_F(FixtureEvaluatorTest, NormalizesInvalidActionDelays) {
  WriteFixture(R"(
ctx.after(-10, () => ctx.dispatch("negative"));
ctx.after(NaN, () => ctx.dispatch("nan"));
ctx.after(Infinity, () => ctx.dispatch("infinity"));
ctx.after(Number.MAX_VALUE, () => ctx.dispatch("overflow"));
)");

  auto result = EvaluateFixture(fixture_directory_);

  ASSERT_TRUE(result.has_value()) << result.error();
  ASSERT_EQ(result.value().actions.size(), 4u);
  EXPECT_EQ(result.value().actions[0].delay_ms, 0);
  EXPECT_EQ(result.value().actions[1].delay_ms, 0);
  EXPECT_EQ(result.value().actions[2].delay_ms, 0);
  EXPECT_EQ(result.value().actions[3].delay_ms,
            std::numeric_limits<int64_t>::max());
}

TEST_F(FixtureEvaluatorTest, EnforcesScriptAndAssetByteBudgets) {
  const std::string source =
      "ctx.sharedData('data', ctx.readAsset('data.txt'));";
  WriteFixture(source);
  WriteAsset("data.txt", "1234");
  FixtureEvaluationLimits limits;
  limits.max_script_bytes = source.size();
  limits.max_asset_bytes = 4;
  auto exact = EvaluateFixture(fixture_directory_, limits);
  ASSERT_TRUE(exact.has_value()) << exact.error();
  ASSERT_EQ(exact->shared_data.size(), 1u);
  EXPECT_EQ(exact->shared_data[0].value_json, "\"1234\"");

  limits.max_asset_bytes = 3;
  auto oversized_asset = EvaluateFixture(fixture_directory_, limits);
  ASSERT_TRUE(oversized_asset.has_value()) << oversized_asset.error();
  EXPECT_EQ(oversized_asset->shared_data[0].value_json, "null");
  --limits.max_script_bytes;
  EXPECT_FALSE(EvaluateFixture(fixture_directory_, limits).has_value());
}

TEST_F(FixtureEvaluatorTest, PreservesDslAndRecordedEventNames) {
  WriteFixture(R"(
ctx.sendGlobalEvent({value: 1});
ctx.sendCustomEvent({value: 2});
ctx.sendTouchEvent({value: 3});
ctx.sendEventAndroid({value: 4});
ctx.reloadTemplate({value: 5});
ctx.sharedData('methods', Object.keys(ctx).sort());
)");
  auto result = EvaluateFixture(fixture_directory_);
  ASSERT_TRUE(result.has_value()) << result.error();
  ASSERT_EQ(result->actions.size(), 5u);
  EXPECT_EQ(result->actions[0].function_name, "sendGlobalEvent");
  EXPECT_EQ(result->actions[1].function_name, "SendCustomEvent");
  EXPECT_EQ(result->actions[2].function_name, "SendTouchEvent");
  EXPECT_EQ(result->actions[3].function_name, "sendEventAndroid");
  EXPECT_EQ(result->actions[4].function_name, "reloadTemplate");
  ASSERT_EQ(result->shared_data.size(), 1u);
  EXPECT_EQ(
      result->shared_data[0].value_json,
      "[\"after\",\"dispatch\",\"loadTemplate\",\"readAsset\",\"register\","
      "\"reloadTemplate\",\"sendCustomEvent\",\"sendEventAndroid\","
      "\"sendGlobalEvent\",\"sendTouchEvent\",\"setGlobalProps\","
      "\"setThreadStrategy\",\"sharedData\",\"updateViewPort\"]");
}

TEST_F(FixtureEvaluatorTest, SupportsPlainScriptsAndReportsEvaluationErrors) {
  WriteFixture("ctx.updateViewPort({width: 320});");
  auto plain_script_result = EvaluateFixture(fixture_directory_);
  ASSERT_TRUE(plain_script_result.has_value()) << plain_script_result.error();
  ASSERT_EQ(plain_script_result.value().actions.size(), 1u);
  EXPECT_EQ(plain_script_result.value().actions[0].function_name,
            "updateViewPort");

  WriteFixture("throw new Error('broken fixture');");
  auto invalid_script_result = EvaluateFixture(fixture_directory_);
  ASSERT_FALSE(invalid_script_result.has_value());
  EXPECT_EQ(invalid_script_result.error(), "fixture.js evaluation failed");
}

TEST_F(FixtureEvaluatorTest, ReportsAfterCallbackErrors) {
  WriteFixture(R"(
try {
  ctx.after(10, () => {
    ctx.dispatch("beforeError");
    throw new Error("broken callback");
  });
} catch (_) {}
ctx.dispatch("afterError");
)");

  auto result = EvaluateFixture(fixture_directory_);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "fixture.js evaluation failed");
}

TEST_F(FixtureEvaluatorTest, InterruptsScriptsThatExceedTheDeadline) {
  WriteFixture("for (let i = 0; i < 10000000; ++i) {}");
  FixtureEvaluationLimits limits;
  limits.timeout_ms = 10;

  auto result = EvaluateFixture(fixture_directory_, limits);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "fixture.js evaluation timed out");
}

TEST_F(FixtureEvaluatorTest, RejectsResultsThatExceedTheOutputBudget) {
  WriteFixture(R"(
try {
  ctx.dispatch("first");
  ctx.dispatch("second");
} catch (_) {}
)");
  FixtureEvaluationLimits limits;
  limits.max_result_entries = 1;

  auto result = EvaluateFixture(fixture_directory_, limits);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "fixture.js evaluation output limit exceeded");
}

TEST_F(FixtureEvaluatorTest, RejectsScriptsThatExceedTheMemoryBudget) {
  WriteFixture(R"(
const values = [];
for (let i = 0; i < 128; ++i) {
  values.push(new ArrayBuffer(65536));
}
)");
  auto result_with_default_limits = EvaluateFixture(fixture_directory_);
  ASSERT_TRUE(result_with_default_limits.has_value())
      << result_with_default_limits.error();

  FixtureEvaluationLimits limits;
  limits.memory_limit_bytes = 4 * 1024 * 1024;

  auto result = EvaluateFixture(fixture_directory_, limits);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "fixture.js evaluation failed");
}

}  // namespace
}  // namespace replay
}  // namespace tasm
}  // namespace lynx
