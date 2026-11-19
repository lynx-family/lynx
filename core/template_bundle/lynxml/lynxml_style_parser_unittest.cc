// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynxml/lynxml_style_parser.h"

#include "core/renderer/css/css_keyframes_token.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace {

class LynxMLStyleParserTest : public ::testing::Test {
 protected:
  LynxMLStyleParseResult Parse(const std::string& style) {
    CompileOptions options;
    options.target_sdk_version_ = "4.0";
    return ParseLynxMLStyle(style, options);
  }
};

TEST_F(LynxMLStyleParserTest, MergesEquivalentKeyframeOffsets) {
  auto result = Parse(R"(
    @keyframes demo {
      0% { opacity: 0; }
      from { transform: scale(.5); }
      to { opacity: 1; }
      100% { transform: scale(1); }
    }
  )");
  ASSERT_TRUE(result.success) << result.error;
  auto* token = result.fragment->GetKeyframesRule(base::String("demo"));
  ASSERT_NE(token, nullptr);
  auto& frames = token->GetKeyframesContent();
  ASSERT_EQ(frames.size(), 2u);
  for (float offset : {0.f, 1.f}) {
    ASSERT_NE(frames.find(offset), frames.end());
    const auto& styles = *frames[offset];
    ASSERT_NE(styles.find(kPropertyIDOpacity), styles.end());
    EXPECT_EQ(styles.find(kPropertyIDOpacity)->second.GetNumber(), offset);
    EXPECT_NE(styles.find(kPropertyIDTransform), styles.end());
  }
}

TEST_F(LynxMLStyleParserTest, OverridesOnlyRepeatedProperties) {
  auto result = Parse(R"(
    @keyframes demo {
      50% { opacity: 0; transform: scale(.5); }
      50% { opacity: .75; }
      50% {}
    }
  )");
  ASSERT_TRUE(result.success) << result.error;
  auto* token = result.fragment->GetKeyframesRule(base::String("demo"));
  ASSERT_NE(token, nullptr);
  auto& frames = token->GetKeyframesContent();
  ASSERT_EQ(frames.size(), 1u);
  ASSERT_NE(frames.find(.5f), frames.end());
  const auto& styles = *frames[.5f];
  ASSERT_NE(styles.find(kPropertyIDOpacity), styles.end());
  EXPECT_EQ(styles.find(kPropertyIDOpacity)->second.GetNumber(), .75);
  EXPECT_NE(styles.find(kPropertyIDTransform), styles.end());
}

TEST_F(LynxMLStyleParserTest, KeepsGroupedOffsetsIndependent) {
  auto result = Parse(R"(
    @keyframes demo {
      0%, 50%, 100% { opacity: .25; }
      from { opacity: 0; transform: scale(.5); }
      to { opacity: 1; }
    }
  )");
  ASSERT_TRUE(result.success) << result.error;
  auto* token = result.fragment->GetKeyframesRule(base::String("demo"));
  ASSERT_NE(token, nullptr);
  auto& frames = token->GetKeyframesContent();
  ASSERT_EQ(frames.size(), 3u);
  for (float offset : {0.f, .5f, 1.f}) {
    ASSERT_NE(frames.find(offset), frames.end());
    ASSERT_NE(frames[offset]->find(kPropertyIDOpacity), frames[offset]->end());
    EXPECT_EQ(frames[offset]->at(kPropertyIDOpacity).GetNumber(),
              offset == .5f ? .25 : offset);
    EXPECT_EQ(
        frames[offset]->find(kPropertyIDTransform) != frames[offset]->end(),
        offset == 0.f);
  }
}

TEST_F(LynxMLStyleParserTest,
       MergesCustomPropertiesWithoutChangingOtherOffsets) {
  auto result = Parse(R"(
    @keyframes demo {
      0%, 50% { --alpha: 0; --scale: .5; }
      from { --alpha: 1; --color: red; }
    }
  )");
  ASSERT_TRUE(result.success) << result.error;
  auto* token = result.fragment->GetKeyframesRule(base::String("demo"));
  ASSERT_NE(token, nullptr);
  auto& frames = token->GetKeyframesCustomPropertyContent();
  ASSERT_EQ(frames.size(), 2u);
  for (float offset : {0.f, .5f}) {
    ASSERT_NE(frames.find(offset), frames.end());
    const auto& properties = *frames[offset];
    ASSERT_NE(properties.find(base::String("--alpha")), properties.end());
    ASSERT_NE(properties.find(base::String("--scale")), properties.end());
    EXPECT_EQ(
        properties.find(base::String("--alpha"))->second.GetValue().StdString(),
        offset == 0.f ? "1" : "0");
    EXPECT_EQ(
        properties.find(base::String("--scale"))->second.GetValue().StdString(),
        ".5");
    EXPECT_EQ(properties.find(base::String("--color")) != properties.end(),
              offset == 0.f);
  }
}

TEST_F(LynxMLStyleParserTest, ReplacesSeparateKeyframesRulesWithTheSameName) {
  auto result = Parse(R"(
    @keyframes demo { from { opacity: 0; } }
    @keyframes demo { from { transform: scale(.5); } }
  )");
  ASSERT_TRUE(result.success) << result.error;
  auto* token = result.fragment->GetKeyframesRule(base::String("demo"));
  ASSERT_NE(token, nullptr);
  auto& frames = token->GetKeyframesContent();
  ASSERT_EQ(frames.size(), 1u);
  ASSERT_NE(frames.find(0.f), frames.end());
  EXPECT_EQ(frames[0.f]->find(kPropertyIDOpacity), frames[0.f]->end());
  EXPECT_NE(frames[0.f]->find(kPropertyIDTransform), frames[0.f]->end());
}

}  // namespace
}  // namespace tasm
}  // namespace lynx
