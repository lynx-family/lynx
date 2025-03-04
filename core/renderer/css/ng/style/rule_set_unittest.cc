// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/style/rule_set.h"

#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

struct TestFragment {
  TestFragment() : rule_set_(nullptr) {}
  void AddCSSRules(const std::string& input) {
    CSSParserContext context;
    CSSTokenizer tokenizer(input);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    size_t flattened_size = CSSSelectorParser::FlattenedSize(vector);
    auto selector_array = std::make_unique<LynxCSSSelector[]>(flattened_size);
    CSSSelectorParser::AdoptSelectorVector(vector, selector_array.get(),
                                           flattened_size);
    rule_set_.AddStyleRule(
        std::make_unique<StyleRule>(std::move(selector_array), nullptr));
  }
  RuleSet& GetRuleSet() { return rule_set_; }
  RuleSet rule_set_;
};

TEST(RuleSetTest, FindBestRuleSetAndAdd_Id) {
  TestFragment fragment;

  fragment.AddCSSRules("#main");
  RuleSet& rule_set = fragment.GetRuleSet();
  std::string str("main");
  auto rules = rule_set.id_rules(str);
  ASSERT_EQ(1u, rules.size());
  ASSERT_EQ(str, rules.front().Selector().Value());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_Nth) {
  TestFragment fragment;

  fragment.AddCSSRules("view:nth-child(1)");
  RuleSet& rule_set = fragment.GetRuleSet();
  std::string str("view");
  auto rules = rule_set.tag_rules(str);
  ASSERT_EQ(1u, rules.size());
  ASSERT_EQ(str, rules.front().Selector().Value());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_Pseudo) {
  TestFragment fragment;

  fragment.AddCSSRules("view:hover");
  RuleSet& rule_set = fragment.GetRuleSet();
  std::string str("view");
  auto rules = rule_set.pseudo_rules();
  ASSERT_EQ(1u, rules.size());
  ASSERT_EQ(str, rules.front().Selector().Value());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_ClassAndId) {
  TestFragment fragment;

  fragment.AddCSSRules(".item#main");
  RuleSet& rule_set = fragment.GetRuleSet();
  std::string str("main");
  auto rules = rule_set.id_rules(str);
  ASSERT_EQ(1u, rules.size());
  std::string class_str("item");
  ASSERT_EQ(class_str, rules.front().Selector().Value());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_IdAndClass) {
  TestFragment fragment;

  fragment.AddCSSRules("#main.item");
  RuleSet& rule_set = fragment.GetRuleSet();
  std::string str("main");
  auto rules = rule_set.id_rules(str);
  ASSERT_EQ(1u, rules.size());
  ASSERT_EQ(str, rules.front().Selector().Value());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_AttrAndId) {
  TestFragment fragment;

  fragment.AddCSSRules("[attr]#main");
  RuleSet& rule_set = fragment.GetRuleSet();
  std::string str("main");
  auto rules = rule_set.id_rules(str);
  ASSERT_EQ(1u, rules.size());
  std::string attr_str("attr");
  ASSERT_EQ(attr_str, rules.front().Selector().Attribute());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_TagAndAttrAndId) {
  TestFragment fragment;

  fragment.AddCSSRules("view[attr]#main");
  RuleSet& rule_set = fragment.GetRuleSet();
  std::string str("main");
  auto rules = rule_set.id_rules(str);
  ASSERT_EQ(1u, rules.size());
  std::string tag_str("view");
  ASSERT_EQ(tag_str, rules.front().Selector().Value());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_TagAndAttr) {
  TestFragment fragment;

  fragment.AddCSSRules("view[attr]");
  RuleSet& rule_set = fragment.GetRuleSet();
  ASSERT_EQ(1u, rule_set.attr_rules("attr").size());
  ASSERT_TRUE(rule_set.tag_rules("view").empty());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_AttrAndClass) {
  TestFragment fragment;

  fragment.AddCSSRules("[attr].item");
  RuleSet& rule_set = fragment.GetRuleSet();
  ASSERT_TRUE(rule_set.attr_rules("attr").empty());
  ASSERT_EQ(1u, rule_set.class_rules("item").size());
}

TEST(RuleSetTest, FindBestRuleSetAndAdd_PlaceholderPseudo) {
  TestFragment fragment;

  fragment.AddCSSRules("::placeholder");
  fragment.AddCSSRules("input::placeholder");
  RuleSet& rule_set = fragment.GetRuleSet();
  auto rules = rule_set.pseudo_rules();
  ASSERT_EQ(2u, rules.size());
}

}  // namespace css
}  // namespace lynx
