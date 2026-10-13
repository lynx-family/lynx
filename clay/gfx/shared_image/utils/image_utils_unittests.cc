// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>

#include "clay/gfx/shared_image/utils/image_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(ImageUtilsTest, CopiesPaddedRows) {
  const uint8_t source[] = {1, 2, 3, 4, 99, 99, 5, 6, 7, 8};
  uint8_t destination[8] = {};
  CopyPlane(source, 6, destination, 4, 4, 2);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(destination[i], i + 1);
  }
}

TEST(ImageUtilsTest, CopiesInvertedRows) {
  const uint8_t source[] = {1, 2, 3, 4};
  uint8_t destination[4] = {};
  CopyPlane(source, 2, destination, 2, 2, -2);
  EXPECT_EQ(destination[0], 3);
  EXPECT_EQ(destination[1], 4);
  EXPECT_EQ(destination[2], 1);
  EXPECT_EQ(destination[3], 2);
}

TEST(ImageUtilsTest, RejectsUnrepresentableHeight) {
  const uint8_t source = 1;
  uint8_t destination = 0;
  CopyPlane(&source, 1, &destination, 1, 1, std::numeric_limits<int>::min());
  EXPECT_EQ(destination, 0);
}

}  // namespace clay
