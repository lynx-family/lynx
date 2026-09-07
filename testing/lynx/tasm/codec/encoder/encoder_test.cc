// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/encoder.h"

#include <algorithm>
#include <string>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace test {
namespace {

constexpr char kMinimalTemplate[] = R"({
  "compilerOptions": {
    "enableFiberArch": true,
    "targetSdkVersion": "2.8",
    "bundleModuleMode": "ReturnByFunction",
    "useLepusNG": true
  },
  "sourceContent": {
    "dsl": "tt",
    "appType": "card",
    "config": {}
  },
  "css": {
    "cssMap": {"0": []},
    "cssSource": {"0": "index.css"}
  },
  "lepusCode": {"root": ""},
  "manifest": {}
})";

TEST(PublicEncoderTest, EncodeMinimalTemplate) {
  auto result = encode(kMinimalTemplate);

  ASSERT_EQ(result.status, 0) << result.error_msg;
  EXPECT_FALSE(result.buffer.empty());
}

TEST(PublicEncoderTest, EncodeTrace) {
  auto result = encode(kMinimalTemplate, true);
  ASSERT_EQ(result.status, 0) << result.error_msg;

  rapidjson::Document trace;
  trace.Parse(result.trace);
  ASSERT_FALSE(trace.HasParseError());
  ASSERT_TRUE(trace.IsArray());

  const std::vector<std::string> expected_events = {
      "ParseJSON", "ParseEncodeOptions", "EncodeTemplate", "Encode"};
  for (const auto& expected_name : expected_events) {
    const auto event = std::find_if(
        trace.Begin(), trace.End(), [&expected_name](const auto& value) {
          return value.IsObject() && value.HasMember("name") &&
                 value["name"].IsString() &&
                 expected_name == value["name"].GetString();
        });
    EXPECT_NE(event, trace.End()) << expected_name;
  }

  auto result_without_trace = encode(kMinimalTemplate);
  ASSERT_EQ(result_without_trace.status, 0) << result_without_trace.error_msg;
  EXPECT_TRUE(result_without_trace.trace.empty());
  EXPECT_EQ(result_without_trace.buffer, result.buffer);
}

TEST(PublicEncoderTest, RejectInvalidInput) {
  auto result = encode("{invalid}", true);

  EXPECT_NE(result.status, 0);
  EXPECT_FALSE(result.error_msg.empty());
  EXPECT_FALSE(result.trace.empty());
}

}  // namespace
}  // namespace test
}  // namespace tasm
}  // namespace lynx
