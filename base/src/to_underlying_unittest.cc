// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/to_underlying.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {

TEST(ToUnderLyingTest, EnumClassTest) {
  enum class E1 : char { e };
  static_assert(std::is_same_v<char, decltype(base::to_underlying(E1::e))>);

  enum class ColorMask : std::uint32_t {
    red = 0xFF,
    green = (red << 8),
    blue = (green << 8),
    alpha = (blue << 8)
  };

  ASSERT_EQ(base::to_underlying(ColorMask::red),
            static_cast<const uint32_t>(0xFF));
  ASSERT_EQ(base::to_underlying(ColorMask::green),
            static_cast<const uint32_t>(0xFF00));
  ASSERT_EQ(base::to_underlying(ColorMask::blue),
            static_cast<const uint32_t>(0xFF0000));
  ASSERT_EQ(base::to_underlying(ColorMask::alpha),
            static_cast<const uint32_t>(0xFF000000));

  [[maybe_unused]] std::underlying_type_t<ColorMask> y =
      base::to_underlying(ColorMask::alpha);  // OK
}

TEST(ToUnderLyingTest, EnumStructTest) {
  enum struct E2 : long { e };
  static_assert(std::is_same_v<long, decltype(base::to_underlying(E2::e))>);

  ASSERT_EQ(base::to_underlying(E2::e), 0);
}

TEST(ToUnderLyingTest, EnumTest) {
  enum E3 : unsigned { e };
  static_assert(std::is_same_v<unsigned, decltype(base::to_underlying(e))>);

  ASSERT_EQ(base::to_underlying(e), static_cast<const uint32_t>(0));
}
}  // namespace lynx
