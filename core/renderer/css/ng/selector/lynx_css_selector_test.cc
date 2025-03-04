// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/selector/lynx_css_selector.h"

#include <memory>
#include <utility>

#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

struct LynxSelectorTestCase {
  // The input string to parse as a selector list.
  const char* input;
};

class LynxSelectorTest : public ::testing::TestWithParam<LynxSelectorTestCase> {
};

TEST_P(LynxSelectorTest, Parse) {
  auto param = GetParam();
  SCOPED_TRACE(param.input);
  CSSParserContext context;
  CSSTokenizer tokenizer(param.input);
  const auto tokens = tokenizer.TokenizeToEOF();
  CSSParserTokenRange range(tokens);
  LynxCSSSelectorVector vector =
      CSSSelectorParser::ParseSelector(range, &context);
  auto list = CSSSelectorParser::AdoptSelectorVector(vector);
  EXPECT_TRUE(list.IsValid());
  // Encode
  auto array = lepus::CArray::Create();
  auto current = list.First();
  while (current) {
    array->push_back(current->ToLepus());
    if (current->IsLastInTagHistory() && current->IsLastInSelectorList()) {
      break;
    }
    current++;
  }

  // Decode
  int flattened_size = array->size();
  auto selector_array = std::make_unique<LynxCSSSelector[]>(flattened_size);
  for (int i = 0; i < flattened_size; ++i) {
    LynxCSSSelector::FromLepus(selector_array[i], array->get(i));
  }
  auto result = LynxCSSSelectorList(std::move(selector_array));

  // EXPECT_EQ(param.input, list.SelectorsText());
  EXPECT_EQ(result.SelectorsText(), list.SelectorsText());
}

static const LynxSelectorTestCase lynx_css_selector_test_data[] = {
    {"div"},
    {":active"},
    {R"CSS(.\[item\])CSS"},
    {".\\[item\\]"},
    {"div.class"},
    {"div:active"},
    {"#list:focus"},
    {"div:hover"},
    {"input::placeholder"},
    {"*::placeholder"},
    {"::selection"},
    {"#text::selection"},
    {"#main"},
    {".a.b.c"},
    {"#main div"},
    {"#main div .a"},
    {"div .a.b"},
    {"view text"},
    {"div *"},
    {"div text *"},
    {"view[flatten=\"false\"]"},
    {"a[title]"},
    {"div text *"},
    {"div text *"},
    {"*"},
};

INSTANTIATE_TEST_SUITE_P(LynxCSSSelectorTest, LynxSelectorTest,
                         testing::ValuesIn(lynx_css_selector_test_data));

}  // namespace css
}  // namespace lynx
