// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/matcher/selector_matcher.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/tasm/testing/mock_attribute_holder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

TEST(CSSMatcherTest, CheckSimple) {
  const char* test_case = "div .a:focus";

  SCOPED_TRACE(test_case);

  CSSParserContext context;
  CSSTokenizer tokenizer(test_case);
  const auto tokens = tokenizer.TokenizeToEOF();
  CSSParserTokenRange range(tokens);
  LynxCSSSelectorVector vector =
      CSSSelectorParser::ParseSelector(range, &context);
  auto list = CSSSelectorParser::AdoptSelectorVector(vector);
  auto parent = std::make_unique<tasm::MockAttributeHolder>("div");
  auto child = std::make_unique<tasm::MockAttributeHolder>("div");
  auto child_ptr = child.get();
  child->SetClass("a");
  child->SetPseudoState(tasm::kPseudoStateFocus);
  parent->AddChild(std::move(child));

  SelectorMatcher matcher;
  SelectorMatcher::SelectorMatchingContext matchingContext(child_ptr);
  matchingContext.selector = list.First();
  auto ret = matcher.Match(matchingContext);

  EXPECT_TRUE(ret);
}

struct MatchStatusTestData {
  const char* selector;
  bool status;
};

MatchStatusTestData matcher_test_data[] = {
    {"*", true},
    {"view", true},
    {":root view", true},
    {".foo", true},
    {".bar", false},
    {"#main", true},
    {"#test", false},
    {":focus", true},
    {":active", false},

    // We never evaluate :hover, since :active fails to match.
    {":active:hover", false},

    // Non-rightmost compound:
    {":focus *", true},
    {":focus > *", true},
    {"text + .foo", true},
    {"view + .foo", false},
    {"text ~ .foo", true},
    {"view ~ .foo", false},

    // Within pseudo-classes:
    {":not(text)", true},
    {":not(view)", false},
    {":not(:active, :hover)", true},
    {":not(:active, :focus)", false},
};

class MatchStatusTest
    : public testing::Test,
      public testing::WithParamInterface<MatchStatusTestData> {};

INSTANTIATE_TEST_SUITE_P(SelectorChecker, MatchStatusTest,
                         testing::ValuesIn(matcher_test_data));

TEST_P(MatchStatusTest, All) {
  MatchStatusTestData param = GetParam();

  CSSParserContext context;
  CSSTokenizer tokenizer(param.selector);
  const auto tokens = tokenizer.TokenizeToEOF();
  CSSParserTokenRange range(tokens);
  LynxCSSSelectorVector vector =
      CSSSelectorParser::ParseSelector(range, &context);
  auto list = CSSSelectorParser::AdoptSelectorVector(vector);

  auto parent = std::make_unique<tasm::MockAttributeHolder>("page");
  parent->SetPseudoState(tasm::kPseudoStateFocus);
  auto first = std::make_unique<tasm::MockAttributeHolder>("text");
  parent->AddChild(std::move(first));

  auto target = std::make_unique<tasm::MockAttributeHolder>("view");
  auto target_ptr = target.get();
  target->SetIdSelector("main");
  target->SetClass("foo");
  target->SetStaticAttribute("flatten", lepus_value("true"));
  target->SetStaticAttribute("color", lepus_value("red green blue"));
  target->SetStaticAttribute("lang", lepus_value("zh-CN"));
  target->SetPseudoState(tasm::kPseudoStateFocus);
  parent->AddChild(std::move(target));

  auto next = std::make_unique<tasm::MockAttributeHolder>("view");
  parent->AddChild(std::move(next));

  auto last = std::make_unique<tasm::MockAttributeHolder>("view");
  parent->AddChild(std::move(last));

  SelectorMatcher matcher;
  SelectorMatcher::SelectorMatchingContext matchingContext(target_ptr);
  matchingContext.selector = list.First();
  bool ret = matcher.Match(matchingContext);
  SCOPED_TRACE(param.selector);
  EXPECT_EQ(param.status, ret);
}

TEST(CSSMatcherTest, CheckPseudoElement) {
  const char* test_case = "div .a::placeholder, div::selection";

  SCOPED_TRACE(test_case);

  CSSParserContext context;
  CSSTokenizer tokenizer(test_case);
  const auto tokens = tokenizer.TokenizeToEOF();
  CSSParserTokenRange range(tokens);
  LynxCSSSelectorVector vector =
      CSSSelectorParser::ParseSelector(range, &context);
  auto list = CSSSelectorParser::AdoptSelectorVector(vector);
  auto parent = std::make_unique<tasm::MockAttributeHolder>("div");
  auto child = std::make_unique<tasm::MockAttributeHolder>("div");
  auto child_ptr = child.get();
  child->SetClass("a");
  parent->AddChild(std::move(child));

  tasm::MockAttributeHolder placeholder("view");
  placeholder.AddPseudoState(tasm::kPseudoStatePlaceHolder);
  placeholder.SetPseudoElementOwner(child_ptr);

  tasm::MockAttributeHolder selection("view");
  selection.AddPseudoState(tasm::kPseudoStateSelection);
  selection.SetPseudoElementOwner(child_ptr);

  {
    SelectorMatcher matcher;
    SelectorMatcher::SelectorMatchingContext matchingContext(child_ptr);
    matchingContext.selector = list.First();
    auto ret = matcher.Match(matchingContext);
    EXPECT_FALSE(ret);
  }
  {
    SelectorMatcher matcher;
    SelectorMatcher::SelectorMatchingContext matchingContext(&placeholder);
    matchingContext.selector = list.First();
    auto ret = matcher.Match(matchingContext);
    EXPECT_TRUE(ret);
  }
  {
    SelectorMatcher matcher;
    SelectorMatcher::SelectorMatchingContext matchingContext(&selection);
    matchingContext.selector = list.Next(*list.First());
    auto ret = matcher.Match(matchingContext);
    EXPECT_TRUE(ret);
  }
}

}  // namespace css
}  // namespace lynx
