// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_evaluator.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <string>
#include <system_error>

#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace replay {
namespace {

std::string SanitizeTestName(std::string name) {
  for (char& character : name) {
    const auto value = static_cast<unsigned char>(character);
    if (!std::isalnum(value) && character != '_' && character != '-') {
      character = '_';
    }
  }
  return name;
}

std::filesystem::path CreateFixtureDirectory(const char* test_name) {
  std::random_device random;
  const std::string prefix =
      "lynx_fixture_evaluator_" + SanitizeTestName(test_name) + "_";
  for (int attempt = 0; attempt < 16; ++attempt) {
    auto candidate = std::filesystem::temp_directory_path() /
                     (prefix + std::to_string(random()));
    std::error_code error;
    if (std::filesystem::create_directory(candidate, error)) {
      return candidate;
    }
  }
  return {};
}

class FixtureEvaluatorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const auto* test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    fixture_directory_ = CreateFixtureDirectory(test_info->name());
    ASSERT_FALSE(fixture_directory_.empty());
    ASSERT_TRUE(
        std::filesystem::create_directory(fixture_directory_ / "assets"));
  }

  void TearDown() override {
    if (!fixture_directory_.empty()) {
      std::error_code error;
      std::filesystem::remove_all(fixture_directory_, error);
    }
  }

  void WriteFixture(const std::string& source) {
    WriteFile(fixture_directory_ / "fixture.js", source);
  }

  void WriteAsset(const std::string& path, const std::string& content) {
    auto file_path = fixture_directory_ / "assets" / path;
    std::filesystem::create_directories(file_path.parent_path());
    WriteFile(file_path, content);
  }

  std::filesystem::path fixture_directory_;

 private:
  static void WriteFile(const std::filesystem::path& path,
                        const std::string& content) {
    std::ofstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    stream << content;
    ASSERT_TRUE(stream.good());
  }
};

TEST_F(FixtureEvaluatorTest, ReportsMissingFixtureScript) {
  auto result = EvaluateFixture(fixture_directory_.string());

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

  auto result = EvaluateFixture(fixture_directory_.string());

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

  auto result = EvaluateFixture(fixture_directory_.string());

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

  auto result = EvaluateFixture(fixture_directory_.string());

  ASSERT_TRUE(result.has_value()) << result.error();
  ASSERT_EQ(result.value().actions.size(), 4u);
  EXPECT_EQ(result.value().actions[0].delay_ms, 0);
  EXPECT_EQ(result.value().actions[1].delay_ms, 0);
  EXPECT_EQ(result.value().actions[2].delay_ms, 0);
  EXPECT_EQ(result.value().actions[3].delay_ms,
            std::numeric_limits<int64_t>::max());
}

TEST_F(FixtureEvaluatorTest, SupportsPlainScriptsAndReportsEvaluationErrors) {
  WriteFixture("ctx.updateViewPort({width: 320});");
  auto plain_script_result = EvaluateFixture(fixture_directory_.string());
  ASSERT_TRUE(plain_script_result.has_value()) << plain_script_result.error();
  ASSERT_EQ(plain_script_result.value().actions.size(), 1u);
  EXPECT_EQ(plain_script_result.value().actions[0].function_name,
            "updateViewPort");

  WriteFixture("throw new Error('broken fixture');");
  auto invalid_script_result = EvaluateFixture(fixture_directory_.string());
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

  auto result = EvaluateFixture(fixture_directory_.string());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "fixture.js evaluation failed");
}

TEST_F(FixtureEvaluatorTest, InterruptsScriptsThatExceedTheDeadline) {
  WriteFixture("for (let i = 0; i < 10000000; ++i) {}");
  FixtureEvaluationLimits limits;
  limits.timeout_ms = 10;

  auto result = EvaluateFixture(fixture_directory_.string(), limits);

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

  auto result = EvaluateFixture(fixture_directory_.string(), limits);

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
  auto result_with_default_limits =
      EvaluateFixture(fixture_directory_.string());
  ASSERT_TRUE(result_with_default_limits.has_value())
      << result_with_default_limits.error();

  FixtureEvaluationLimits limits;
  limits.memory_limit_bytes = 4 * 1024 * 1024;

  auto result = EvaluateFixture(fixture_directory_.string(), limits);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "fixture.js evaluation failed");
}

}  // namespace
}  // namespace replay
}  // namespace tasm
}  // namespace lynx
