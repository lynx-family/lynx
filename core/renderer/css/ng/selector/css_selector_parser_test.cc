// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/renderer/css/ng/selector/css_selector_parser.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

typedef struct {
  const char* input;
  const int a;
  const int b;
} ANPlusBTestCase;

struct SelectorTestCase {
  // The input string to parse as a selector list.
  const char* input;

  // The expected serialization of the parsed selector list. If nullptr, then
  // the expected serialization is the same as the input value.
  //
  // For selector list that are expected to fail parsing, use the empty
  // string "".
  const char* expected = nullptr;
};

class SelectorParseTest : public ::testing::TestWithParam<SelectorTestCase> {};

TEST_P(SelectorParseTest, Parse) {
  auto param = GetParam();
  SCOPED_TRACE(param.input);
  CSSParserContext context;
  CSSTokenizer tokenizer(param.input);
  const auto tokens = tokenizer.TokenizeToEOF();
  CSSParserTokenRange range(tokens);
  LynxCSSSelectorVector vector =
      CSSSelectorParser::ParseSelector(range, &context);
  auto list = CSSSelectorParser::AdoptSelectorVector(vector);
  const char* expected = param.expected ? param.expected : param.input;
  EXPECT_EQ(std::string(expected), list.SelectorsText());
}

TEST(CSSSelectorParserTest, SimpleSelector) {
  const char* test_cases[] = {
      "div .a:hover",
  };

  for (auto* test_case : test_cases) {
    SCOPED_TRACE(test_case);

    CSSParserContext context;
    CSSTokenizer tokenizer(test_case);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    auto list = CSSSelectorParser::AdoptSelectorVector(vector);
  }
}

TEST(CSSSelectorParserTest, ValidANPlusB) {
  ANPlusBTestCase test_cases[] = {
      {"odd", 2, 1},
      {"OdD", 2, 1},
      {"even", 2, 0},
      {"EveN", 2, 0},
      {"0", 0, 0},
      {"8", 0, 8},
      {"+12", 0, 12},
      {"-14", 0, -14},

      {"0n", 0, 0},
      {"16N", 16, 0},
      {"-19n", -19, 0},
      {"+23n", 23, 0},
      {"n", 1, 0},
      {"N", 1, 0},
      {"+n", 1, 0},
      {"-n", -1, 0},
      {"-N", -1, 0},

      {"6n-3", 6, -3},
      {"-26N-33", -26, -33},
      {"n-18", 1, -18},
      {"+N-5", 1, -5},
      {"-n-7", -1, -7},

      {"0n+0", 0, 0},
      {"10n+5", 10, 5},
      {"10N +5", 10, 5},
      {"10n -5", 10, -5},
      {"N+6", 1, 6},
      {"n +6", 1, 6},
      {"+n -7", 1, -7},
      {"-N -8", -1, -8},
      {"-n+9", -1, 9},

      {"33N- 22", 33, -22},
      {"+n- 25", 1, -25},
      {"N- 46", 1, -46},
      {"n- 0", 1, 0},
      {"-N- 951", -1, -951},
      {"-n- 951", -1, -951},

      {"29N + 77", 29, 77},
      {"29n - 77", 29, -77},
      {"+n + 61", 1, 61},
      {"+N - 63", 1, -63},
      {"+n/**/- 48", 1, -48},
      {"-n + 81", -1, 81},
      {"-N - 88", -1, -88},

      {"3091970736n + 1", std::numeric_limits<int>::max(), 1},
      {"-3091970736n + 1", std::numeric_limits<int>::min(), 1},
      // B is calculated as +ve first, then negated.
      {"N- 3091970736", 1, -std::numeric_limits<int>::max()},
      {"N+ 3091970736", 1, std::numeric_limits<int>::max()},
  };

  for (auto test_case : test_cases) {
    SCOPED_TRACE(test_case.input);

    std::pair<int, int> ab;
    CSSTokenizer tokenizer(test_case.input);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    bool passed = CSSSelectorParser::ConsumeANPlusB(range, ab);
    EXPECT_TRUE(passed);
    EXPECT_EQ(test_case.a, ab.first);
    EXPECT_EQ(test_case.b, ab.second);
  }
}

TEST(CSSSelectorParserTest, InvalidANPlusB) {
  // Some of these have token range prefixes which are valid <an+b> and could
  // in theory be valid in consumeANPlusB, but this behaviour isn't needed
  // anywhere and not implemented.
  const char* test_cases[] = {
      " odd",     "+ n",     "3m+4",  "12n--34",  "12n- -34",
      "12n- +34", "23n-+43", "10n 5", "10n + +5", "10n + -5",
  };

  for (auto* test_case : test_cases) {
    SCOPED_TRACE(test_case);

    std::pair<int, int> ab;
    CSSTokenizer tokenizer(test_case);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    bool passed = CSSSelectorParser::ConsumeANPlusB(range, ab);
    EXPECT_FALSE(passed);
  }
}

TEST(CSSSelectorParserTest, PseudoElementsInCompoundLists) {
  const char* test_cases[] = {":not(::before)",
                              ":not(::content)",
                              ":host(::before)",
                              ":host(::content)",
                              ":host-context(::before)",
                              ":host-context(::content)",
                              ":-webkit-any(::after, ::before)",
                              ":-webkit-any(::content, span)"};

  CSSParserContext context;

  for (auto* test_case : test_cases) {
    CSSTokenizer tokenizer(test_case);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    EXPECT_EQ(vector.size(), 0u);
  }
}

TEST(CSSSelectorParserTest, InvalidSimpleAfterPseudoElementInCompound) {
  const char* test_cases[] = {
      "::before#id",
      "::after:hover",
      ".class::content::before",
      "::shadow.class",
      "::selection:window-inactive::before",
      "::-webkit-volume-slider.class",
      "::before:not(.a)",
      "::shadow:not(::after)",
      "::-webkit-scrollbar:vertical:not(:first-child)",
      "video::-webkit-media-text-track-region-container.scrolling",
      "div ::before.a",
      "::slotted(div):hover",
      "::slotted(div)::slotted(span)",
      "::slotted(div)::before:hover",
      "::slotted(div)::before::slotted(span)",
      "::slotted(*)::first-letter",
      "::slotted(.class)::first-line",
      "::slotted([attr])::-webkit-scrollbar"};

  for (auto* test_case : test_cases) {
    CSSTokenizer tokenizer(test_case);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    CSSParserContext context;
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    EXPECT_EQ(vector.size(), 0u);
  }
}

TEST(CSSSelectorParserTest, InvalidPseudoElementInNonRightmostCompound) {
  const char* test_cases[] = {"::-webkit-volume-slider *", "::before *",
                              "::-webkit-scrollbar *", "::cue *",
                              "::selection *"};

  for (auto* test_case : test_cases) {
    CSSTokenizer tokenizer(test_case);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    CSSParserContext context;
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    EXPECT_EQ(vector.size(), 0u);
  }
}

TEST(CSSSelectorParserTest, UnexpectedPipe) {
  const char* test_cases[] = {"div | .c", "| div", " | div"};

  CSSParserContext context;

  for (auto* test_case : test_cases) {
    CSSTokenizer tokenizer(test_case);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    EXPECT_EQ(vector.size(), 0u);
  }
}

TEST(CSSSelectorParserTest, SerializedUniversal) {
  const char* test_cases[][2] = {
      {"*", "*"},
  };

  CSSParserContext context;

  for (auto** test_case : test_cases) {
    SCOPED_TRACE(test_case[0]);
    CSSTokenizer tokenizer(test_case[0]);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    LynxCSSSelectorList list = CSSSelectorParser::AdoptSelectorVector(vector);
    EXPECT_TRUE(list.IsValid());
    EXPECT_EQ(test_case[1], list.SelectorsText());
  }
}

TEST(CSSSelectorParserTest, AttributeSelectorUniversalInvalid) {
  const char* test_cases[] = {"[*]", "[*|*]"};

  CSSParserContext context;
  for (auto* test_case : test_cases) {
    SCOPED_TRACE(test_case);
    CSSTokenizer tokenizer(test_case);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    EXPECT_EQ(vector.size(), 0u);
  }
}

namespace {

const auto TagLocalName = [](const LynxCSSSelector* selector) {
  return selector->Value();
};

const auto AttributeLocalName = [](const LynxCSSSelector* selector) {
  return selector->Attribute();
};

const auto SelectorValue = [](const LynxCSSSelector* selector) {
  return selector->Value();
};

struct ASCIILowerTestCase {
  const char* input;
  const char16_t* expected;
  std::function<std::string(const LynxCSSSelector*)> getter;
};

}  // namespace

TEST(CSSSelectorParserTest, ASCIILowerHTMLStrict) {
  const ASCIILowerTestCase test_cases[] = {
      {"\\212a bd", u"\u212abd", TagLocalName},
      {"[\\212al-ass]", u"\u212al-ass", AttributeLocalName},
      {".\\212al-ass", u"\u212al-ass", SelectorValue},
      {"#\\212al-ass", u"\u212al-ass", SelectorValue}};

  CSSParserContext context;

  for (auto test_case : test_cases) {
    SCOPED_TRACE(test_case.input);
    CSSTokenizer tokenizer(test_case.input);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    EXPECT_GT(vector.size(), 0u);
    LynxCSSSelectorList list = CSSSelectorParser::AdoptSelectorVector(vector);
    EXPECT_TRUE(list.IsValid());
    const LynxCSSSelector* selector = list.First();
    ASSERT_TRUE(selector);
    EXPECT_EQ(test_case.expected,
              ustring_helper::from_string(test_case.getter(selector)));
  }
}

TEST(CSSSelectorParserTest, ImplicitShadowCrossingCombinators) {
  struct ShadowCombinatorTest {
    const char* input;
    std::vector<std::pair<std::string, LynxCSSSelector::RelationType>>
        expectation;
  };

  const ShadowCombinatorTest test_cases[] = {
      {
          "*::placeholder",
          {
              {"placeholder", LynxCSSSelector::kUAShadow},
              {CSSGlobalStarString(), LynxCSSSelector::kSubSelector},
          },
      },
      {
          "::selection",
          {
              {"selection", LynxCSSSelector::kUAShadow},
              {CSSGlobalStarString(), LynxCSSSelector::kSubSelector},
          },
      },
  };

  CSSParserContext context;

  for (auto test_case : test_cases) {
    SCOPED_TRACE(test_case.input);
    CSSTokenizer tokenizer(test_case.input);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector vector =
        CSSSelectorParser::ParseSelector(range, &context);
    LynxCSSSelectorList list = CSSSelectorParser::AdoptSelectorVector(vector);
    EXPECT_TRUE(list.IsValid());
    const LynxCSSSelector* selector = list.First();
    for (auto sub_expectation : test_case.expectation) {
      ASSERT_TRUE(selector);
      std::string selector_value = selector->Value();
      EXPECT_EQ(sub_expectation.first, selector_value);
      EXPECT_EQ(sub_expectation.second, selector->Relation());
      selector = selector->TagHistory();
    }
    EXPECT_FALSE(selector);
  }
}

static const SelectorTestCase lynx_css_selector_parser_test_data[] = {
    {"div", "div"},
    {":active", ":active"},
    {R"CSS(.\[item\])CSS", ".[item]"},
    {".\\[item\\]", ".[item]"},
    {"div.class", "div.class"},
    {"div:active", "div:active"},
    {"#list:focus", "#list:focus"},
    {"div:hover", "div:hover"},
    {"input::placeholder", "input::placeholder"},
    {"::selection", "::selection"},
    {"#text::selection", "#text::selection"},
    {"#main", "#main"},
    {".a.b.c", ".a.b.c"},
    {"#main div", "#main div"},
    {"#main div .a", "#main div .a"},
    {"div .a.b", "div .a.b"},
    {"view text", "view text"},
    {"div *", "div *"},
    {"div text *", "div text *"},
    // {"view[flatten=\"false\"]", "view[flatten=false]"},
    // {"a[title]", "a[title]"},
    {"div text *", "div text *"},
    {"div text *", "div text *"},
    {"*", "*"},
};

INSTANTIATE_TEST_SUITE_P(LynxCSSSelectorParserTest, SelectorParseTest,
                         testing::ValuesIn(lynx_css_selector_parser_test_data));

}  // namespace css
}  // namespace lynx
