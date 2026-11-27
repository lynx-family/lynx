// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/list_item_element_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {

TEST(ListCSSValueLinkTest, DestroysAnimatedValuesWithoutCSSRuntime) {
  tasm::CSSValue number(1, tasm::CSSValuePattern::NUMBER);
  EXPECT_TRUE(number.IsNumber());
  tasm::CSSValue boolean(true);
  EXPECT_TRUE(boolean.IsBoolean());
  tasm::CSSValue enumeration(tasm::CSSFunctionType::DEFAULT);
  EXPECT_TRUE(enumeration.IsEnum());
  tasm::CSSValue empty;
  EXPECT_TRUE(empty.IsEmpty());
}

}  // namespace list
}  // namespace lynx
