// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <Foundation/Foundation.h>

#include "clay/fml/platform/darwin/string_range_sanitization.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(StringRangeSanitizationTest, CanHandleUnicode) {
  auto result = fml::RangeForCharacterAtIndex(@"😠", 1);
  EXPECT_EQ(result.location, 0UL);
  EXPECT_EQ(result.length, 2UL);
}

TEST(StringRangeSanitizationTest, HandlesInvalidRanges) {
  auto ns_not_found = static_cast<unsigned long>(NSNotFound);
  EXPECT_EQ(fml::RangeForCharacterAtIndex(@"😠", 3).location, ns_not_found);
  EXPECT_EQ(fml::RangeForCharacterAtIndex(@"😠", -1).location, ns_not_found);
  EXPECT_EQ(fml::RangeForCharacterAtIndex(nil, 0).location, ns_not_found);
  EXPECT_EQ(fml::RangeForCharactersInRange(@"😠", NSMakeRange(1, 2)).location, ns_not_found);
  EXPECT_EQ(fml::RangeForCharactersInRange(@"😠", NSMakeRange(3, 0)).location, ns_not_found);
  EXPECT_EQ(fml::RangeForCharactersInRange(nil, NSMakeRange(0, 0)).location, ns_not_found);
}

TEST(StringRangeSanitizationTest, CanHandleUnicodeRange) {
  auto result = fml::RangeForCharactersInRange(@"😠", NSMakeRange(1, 0));
  EXPECT_EQ(result.location, 0UL);
  EXPECT_EQ(result.length, 0UL);
}

TEST(StringRangeSanitizationTest, HandlesEndOfRange) {
  EXPECT_EQ(fml::RangeForCharacterAtIndex(@"1234", 4).location, 4UL);
  EXPECT_EQ(fml::RangeForCharacterAtIndex(@"1234", 4).length, 0UL);
}
