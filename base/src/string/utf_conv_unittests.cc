// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <list>
#include <string>

#include "base/include/string/string_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

#define UTF_EQUAL_PAIR(x) {u8##x, u##x},

TEST(UtfConv, LegalEncoding) {
  const std::list<std::pair<std::string, std::u16string>> pairs = {
      UTF_EQUAL_PAIR("") UTF_EQUAL_PAIR("123") UTF_EQUAL_PAIR("中文")
          UTF_EQUAL_PAIR("123中文") UTF_EQUAL_PAIR("😀")
              UTF_EQUAL_PAIR("中文和emoji:😀👨‍👩‍👧‍👦")};
  for (const auto& item : pairs) {
    EXPECT_EQ(item.first, Utf16ToUtf8(item.second));
    EXPECT_EQ(item.second, Utf8ToUtf16(item.first));
  }
}

TEST(UtfConv, IllegalU8Encoding) {
  std::string legal = u8"😀smile";
  EXPECT_EQ(u"\xFFFD", Utf8ToUtf16(legal.substr(0, 1)));
  EXPECT_EQ(u"\xFFFD", Utf8ToUtf16(legal.substr(3, 1)));

  EXPECT_EQ(u"😀s", Utf8ToUtf16(legal.substr(0, 5)));
  EXPECT_EQ(u"\xFFFD\xFFFD\xFFFDsm", Utf8ToUtf16(legal.substr(1, 5)));
  EXPECT_EQ(u"\xFFFD\xFFFDsmi", Utf8ToUtf16(legal.substr(2, 5)));
  EXPECT_EQ(u"\xFFFDsmil", Utf8ToUtf16(legal.substr(3, 5)));
  EXPECT_EQ(u"smile", Utf8ToUtf16(legal.substr(4, 5)));
}

// Just test it won't crash.
TEST(UtfConv, IllegalU16Encoding) {
  std::u16string legal = u"👨‍👩‍👧‍👦smile";
  // Start with legal but end with illegal.
  Utf16ToUtf8(legal.substr(0, 1));
  // Start with illegal unit.
  Utf16ToUtf8(legal.substr(1, 1));
  // All legal.
  Utf16ToUtf8(legal.substr(0, 5));
}

}  // namespace base
}  // namespace lynx
