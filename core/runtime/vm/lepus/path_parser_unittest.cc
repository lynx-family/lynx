// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/path_parser.h"

#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {

TEST(LepusPathParserTest, BasicUsageTest) {
  base::Vector<std::string> res{"a", "b", "c"};
  ASSERT_EQ(lepus::ParseValuePath("a.b.c"), res);

  res = {"a", "1", "1"};
  ASSERT_EQ(lepus::ParseValuePath("a[1]1"), res);
  ASSERT_EQ(lepus::ParseValuePath("a.[1].1"), res);

  res = {"a", "c1", "0"};
  ASSERT_EQ(lepus::ParseValuePath("a.c1[0]"), res);

  res = {"aa", "b", "1", "c"};
  ASSERT_EQ(lepus::ParseValuePath("aa.b[1]c"), res);

  res = {"a", "b", "1", "ccc"};
  ASSERT_EQ(lepus::ParseValuePath("a.b.[1].ccc"), res);

  res = {"a", "b", "1", "c"};
  ASSERT_EQ(lepus::ParseValuePath("a.b[1]c"), res);

  res = {"a", "b", "1", "2", "123", "4"};
  ASSERT_EQ(lepus::ParseValuePath("a.b[1].[2]123[4]"), res);

  res = {"a", "b", "1", "2", "3"};
  ASSERT_EQ(lepus::ParseValuePath("a.b[1][2].[3]"), res);

  res = {"a", "bbb", "1", "c", "2", "d", "3"};
  ASSERT_EQ(lepus::ParseValuePath("a.bbb[1]c[2].d[3]"), res);
}

TEST(LepusPathParserTest, StartsWithBracketsTest) {
  base::Vector<std::string> res{};
  ASSERT_EQ(lepus::ParseValuePath("["), res);
  ASSERT_EQ(lepus::ParseValuePath("[1"), res);
  ASSERT_EQ(lepus::ParseValuePath("[1]"), res);
}

TEST(LepusPathParserTest, BracketsNotMatchTest) {
  base::Vector<std::string> res{};
  ASSERT_EQ(lepus::ParseValuePath("a.b.c["), res);
  ASSERT_EQ(lepus::ParseValuePath("a.b.c[1"), res);
  ASSERT_EQ(lepus::ParseValuePath("a[1"), res);
  ASSERT_EQ(lepus::ParseValuePath("a.[1"), res);
}

TEST(LepusPathParserTest, NonNumberInBracketsTest) {
  base::Vector<std::string> res{};
  ASSERT_EQ(lepus::ParseValuePath("a.[b]"), res);
  ASSERT_EQ(lepus::ParseValuePath("a[b]"), res);
  ASSERT_EQ(lepus::ParseValuePath("a.[b].[c]"), res);
}

TEST(LepusPathParserTest, MultiBracketsTest) {
  base::Vector<std::string> res{};
  ASSERT_EQ(lepus::ParseValuePath("a.[[[[1]]]]"), res);
  ASSERT_EQ(lepus::ParseValuePath("a.[[[1]]"), res);
  ASSERT_EQ(lepus::ParseValuePath("a[[1]]]]"), res);
}

TEST(LepusPathParserTest, EscapeCharTest) {
  base::Vector<std::string> res{};

  res = {"\\a", "b.c"};
  ASSERT_EQ(lepus::ParseValuePath("\\a.b\\.c"), res);

  res = {"a", "1", "\\"};
  ASSERT_EQ(lepus::ParseValuePath("a[\\1]\\"), res);

  res = {"a", "[1"};
  ASSERT_EQ(lepus::ParseValuePath("a.\\[1"), res);

  res = {"a", "c\\10]"};
  ASSERT_EQ(lepus::ParseValuePath("a.c\\10\\]"), res);

  res = {"a", "b", "1", "c"};
  ASSERT_EQ(lepus::ParseValuePath("a.b[\\\\1]c"), res);

  res = {"a", "b.[1]", "c"};
  ASSERT_EQ(lepus::ParseValuePath("a.b\\.\\[1\\].c"), res);

  res = {"aa\\a", "b", "1", "c"};
  ASSERT_EQ(lepus::ParseValuePath("aa\\a.b[1]c"), res);

  res = {"a", "\\b", "1", "2"};
  ASSERT_EQ(lepus::ParseValuePath("a.\\b[1].[2]"), res);

  res = {"a", "b", "1", "2", ".", "3"};
  ASSERT_EQ(lepus::ParseValuePath("a.b[1][\\2]\\.[3]"), res);
}

}  // namespace lynx
