// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/path.h"
#include "gtest/gtest.h"

namespace clay {

TEST(PathBuilderTest, RelativeClosedPathPreservesMotionPositionAndDirection) {
  GrPath path;
  ASSERT_TRUE(PathBuilder::ParsePathString("M10 20l30 0v40h-30z", &path));
  auto state = PathBuilder::CalculateMotionState(path, 0.25f);
  EXPECT_FLOAT_EQ(state.x, 40);
  EXPECT_FLOAT_EQ(state.y, 25);
  EXPECT_FLOAT_EQ(state.deg, 90);
  state = PathBuilder::CalculateMotionState(path, 0.75f);
  EXPECT_FLOAT_EQ(state.x, 10);
  EXPECT_FLOAT_EQ(state.y, 55);
  EXPECT_FLOAT_EQ(state.deg, -90);
}

TEST(PathBuilderTest, MotionProgressTraversesIndependentContours) {
  GrPath path;
  ASSERT_TRUE(PathBuilder::ParsePathString("M0 0L10 0M20 30L20 50", &path));
  const auto middle = PathBuilder::CalculateMotionState(path, 0.5f);
  EXPECT_FLOAT_EQ(middle.x, 20);
  EXPECT_FLOAT_EQ(middle.y, 35);
  EXPECT_FLOAT_EQ(middle.deg, 90);
  const auto end = PathBuilder::CalculateMotionState(path, 1);
  EXPECT_FLOAT_EQ(end.x, 20);
  EXPECT_FLOAT_EQ(end.y, 50);
}

TEST(PathBuilderTest, FailedParsePreservesExistingGeometry) {
  GrPath path;
  ASSERT_TRUE(PathBuilder::ParsePathString("M0 0L10 0", &path));
  EXPECT_FALSE(PathBuilder::ParsePathString("M1 2L", &path));
  const auto state = PathBuilder::CalculateMotionState(path, 0.5f);
  EXPECT_FLOAT_EQ(state.x, 5);
  EXPECT_FLOAT_EQ(state.y, 0);
  EXPECT_FLOAT_EQ(state.deg, 0);
}

}  // namespace clay
