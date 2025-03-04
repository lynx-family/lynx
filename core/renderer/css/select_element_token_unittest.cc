// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/select_element_token.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

bool operator==(const SelectElementToken &l, const SelectElementToken &r) {
  return l.selector_string == r.selector_string &&
         l.combinator_to_next == r.combinator_to_next && l.type == r.type;
}
namespace testing {

class SelectElementTokenTest : public ::testing::Test {
 public:
  SelectElementTokenTest() {}
  ~SelectElementTokenTest() override {}

  void SetUp() override {}
};

TEST_F(SelectElementTokenTest, ParseEmpty) {
  auto actual_pair = SelectElementToken::ParseCssSelector("");
  auto expected = std::vector<SelectElementToken>{};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseId) {
  auto actual_pair = SelectElementToken::ParseCssSelector(" #id  ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken("#id", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseClass) {
  auto actual_pair = SelectElementToken::ParseCssSelector(" .class1.class2  ");
  auto expected = std::vector<SelectElementToken>{SelectElementToken(
      ".class1.class2", SelectElementToken::Type::CSS_SELECTOR,
      SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseChild) {
  auto actual_pair = SelectElementToken::ParseCssSelector(" #id>.class1 ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken("#id", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::CHILD),
      SelectElementToken(".class1", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseDescendant) {
  auto actual_pair = SelectElementToken::ParseCssSelector(" #id .class1 ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken("#id", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::DESCENDANT),
      SelectElementToken(".class1", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseTag) {
  auto actual_pair = SelectElementToken::ParseCssSelector("tag .class1 ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken("tag", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::DESCENDANT),
      SelectElementToken(".class1", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseTag2) {
  auto actual_pair = SelectElementToken::ParseCssSelector(" .class1 tag");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken(".class1", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::DESCENDANT),
      SelectElementToken("tag", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseTagAndClass) {
  auto actual_pair =
      SelectElementToken::ParseCssSelector("tag.class3 .class1 ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken("tag.class3", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::DESCENDANT),
      SelectElementToken(".class1", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseDescendantAC) {
  auto actual_pair = SelectElementToken::ParseCssSelector(" #id >>>.class1 ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken(
          "#id", SelectElementToken::Type::CSS_SELECTOR,
          SelectElementToken::Combinator::DESCENDANT_ACROSS_COMPONENTS),
      SelectElementToken(".class1", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseAttr1) {
  auto actual_pair = SelectElementToken::ParseCssSelector(" [attr]  ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken("[attr]", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST),
  };
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseAttr2) {
  auto actual_pair = SelectElementToken::ParseCssSelector(" [attr=value]  ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken("[attr=value]", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST),
  };
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, ParseMixed) {
  auto actual_pair =
      SelectElementToken::ParseCssSelector("#id >>>.class1  #id2  >.class2  ");
  auto expected = std::vector<SelectElementToken>{
      SelectElementToken(
          "#id", SelectElementToken::Type::CSS_SELECTOR,
          SelectElementToken::Combinator::DESCENDANT_ACROSS_COMPONENTS),
      SelectElementToken(".class1", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::DESCENDANT),
      SelectElementToken("#id2", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::CHILD),
      SelectElementToken(".class2", SelectElementToken::Type::CSS_SELECTOR,
                         SelectElementToken::Combinator::LAST)};
  EXPECT_EQ(actual_pair.first, expected);
  EXPECT_EQ(actual_pair.second, true);
}

TEST_F(SelectElementTokenTest, Error) {
  auto actual_pair = SelectElementToken::ParseCssSelector("#");
  EXPECT_EQ(actual_pair.second, false);

  actual_pair = SelectElementToken::ParseCssSelector("# id");
  EXPECT_EQ(actual_pair.second, false);

  actual_pair = SelectElementToken::ParseCssSelector(" #id >> .class ");
  EXPECT_EQ(actual_pair.second, false);

  actual_pair = SelectElementToken::ParseCssSelector(" #id >>>> .class ");
  EXPECT_EQ(actual_pair.second, false);

  actual_pair = SelectElementToken::ParseCssSelector(" #id >>>");
  EXPECT_EQ(actual_pair.second, false);

  actual_pair = SelectElementToken::ParseCssSelector(" #id >");
  EXPECT_EQ(actual_pair.second, false);

  actual_pair = SelectElementToken::ParseCssSelector(" > #id");
  EXPECT_EQ(actual_pair.second, false);

  actual_pair = SelectElementToken::ParseCssSelector(" $id");
  EXPECT_EQ(actual_pair.second, false);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
