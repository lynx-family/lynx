// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/string/unicode_decode_utils.h"

#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(UnicodeDecodeUtilsTest, CjkWordJoinerHandlesMalformedUtf8Span) {
  std::string input;
  input.push_back('x');
  const char cjk[] = {static_cast<char>(0xE4), static_cast<char>(0xB8),
                      static_cast<char>(0x80)};
  const char malformed[] = {static_cast<char>(0xE4), static_cast<char>(0xB8),
                            static_cast<char>(0x80), static_cast<char>(0x80),
                            static_cast<char>(0x80)};
  input.append(cjk, sizeof(cjk));
  input.append(malformed, sizeof(malformed));
  input.push_back('a');

  const auto output = UnicodeDecodeUtils::Decode(
      input, UnicodeDecodeProperty::kCjkInsertWordJoiner);

  std::string expected;
  expected.push_back('x');
  expected.append(cjk, sizeof(cjk));
  expected.append("\u2060");
  expected.append(malformed, sizeof(malformed));
  expected.push_back('a');
  EXPECT_EQ(output, expected);
}

}  // namespace base
}  // namespace lynx
