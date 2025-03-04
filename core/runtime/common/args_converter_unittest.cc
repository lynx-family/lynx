// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/args_converter.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace testing {
namespace {
struct Src {
  int value;
};

struct Dest {
  int value;

  Dest() : value(0) {}
  explicit Dest(int value) : value(value) {}
};
}  // namespace

TEST(ARGS_CONVERTER_TEST, StackTest) {
  constexpr int size = 3;
  Src args[size]{{1}, {2}, {3}};
  auto converter = ArgsConverter<Dest, const Src *>(
      size, args, [](const auto &src) { return Dest(src.value + 1); });

  EXPECT_EQ(converter[0].value, 2);
  EXPECT_EQ(converter[1].value, 3);
  EXPECT_EQ(converter[2].value, 4);
}

TEST(ARGS_CONVERTER_TEST, HeapTest) {
  constexpr int size = 12;
  Src args[size]{{1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}, {10}, {11}, {12}};
  auto converter = ArgsConverter<Dest, const Src *>(
      size, args, [](const auto &src) { return Dest(src.value + 1); });

  EXPECT_EQ(converter[0].value, 2);
  EXPECT_EQ(converter[1].value, 3);
  EXPECT_EQ(converter[2].value, 4);
  EXPECT_EQ(converter[3].value, 5);
  EXPECT_EQ(converter[4].value, 6);
  EXPECT_EQ(converter[5].value, 7);
  EXPECT_EQ(converter[6].value, 8);
  EXPECT_EQ(converter[7].value, 9);
  EXPECT_EQ(converter[8].value, 10);
  EXPECT_EQ(converter[9].value, 11);
  EXPECT_EQ(converter[10].value, 12);
  EXPECT_EQ(converter[11].value, 13);
}

}  // namespace testing
}  // namespace piper
}  // namespace lynx
