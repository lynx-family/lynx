// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynxml/lynxml_style_parser.h"

#include "core/renderer/css/css_property_id.h"
#include "core/renderer/css/ng/style/rule_set.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace {

CompileOptions MakeCompileOptions() {
  CompileOptions compile_options;
  compile_options.target_sdk_version_ = "3.5";
  return compile_options;
}

fml::RefPtr<CSSParseToken> ClassRuleToken(SharedCSSFragment* fragment,
                                          const std::string& class_name,
                                          size_t index = 0) {
  if (!fragment || !fragment->rule_set()) {
    return nullptr;
  }
  const auto& rules = fragment->rule_set()->class_rules(class_name);
  return index < rules.size() ? rules[index].Rule()->Token() : nullptr;
}

TEST(LynxMLStyleParserTest, ParsesQualifiedRules) {
  auto result = ParseLynxMLStyle(
      R"(
        .card {
          width: calc(10px + 20px);
          color: red !important;
          --brand-color: blue;
          font-family: "a;b";
        }
        #hero { height: 200px; }
        view { opacity: 0.5; }
      )",
      MakeCompileOptions());

  ASSERT_TRUE(result.success) << result.error;
  ASSERT_NE(result.fragment, nullptr);
  auto* rule_set = result.fragment->rule_set();
  ASSERT_NE(rule_set, nullptr);
  EXPECT_EQ(rule_set->class_rules("card").size(), 1u);
  EXPECT_EQ(rule_set->id_rules("hero").size(), 1u);
  EXPECT_EQ(rule_set->tag_rules("view").size(), 1u);

  auto token = ClassRuleToken(result.fragment.get(), "card");
  ASSERT_NE(token, nullptr);
  EXPECT_TRUE(token->GetAttributes().contains(kPropertyIDWidth));
  EXPECT_TRUE(token->GetAttributes().contains(kPropertyIDFontFamily));
  EXPECT_TRUE(token->GetImportantAttributes().contains(kPropertyIDColor));
  auto variable =
      token->GetStyleVariables().find(base::String("--brand-color"));
  ASSERT_NE(variable, token->GetStyleVariables().end());
  EXPECT_EQ(variable->second, base::String("blue"));
}

TEST(LynxMLStyleParserTest, RejectsInvalidQualifiedRules) {
  const char* styles[] = {
      ".a, { width: 1px; }",
      ".a { width nope; }",
      ".a { width: 1px; ",
  };

  for (const char* style : styles) {
    auto result = ParseLynxMLStyle(style, MakeCompileOptions());
    EXPECT_FALSE(result.success) << style;
    EXPECT_FALSE(result.error.empty()) << style;
  }
}

}  // namespace
}  // namespace tasm
}  // namespace lynx
