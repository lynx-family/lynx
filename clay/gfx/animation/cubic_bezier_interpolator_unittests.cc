// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/cubic_bezier_interpolator.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(CubicBezierInterpolatorTest, Basic) {
  float tolerance = 0.01f;
  std::unique_ptr<Interpolator> cubic_ease_timing =
      CubicBezierInterpolator::CreatePreset(CubicBezierInterpolator::Ease);
  EXPECT_NEAR(0.418f, cubic_ease_timing->Interpolate(0.25), tolerance);
  EXPECT_NEAR(0.805f, cubic_ease_timing->Interpolate(0.50), tolerance);
  EXPECT_NEAR(0.960f, cubic_ease_timing->Interpolate(0.75), tolerance);

  std::unique_ptr<Interpolator> cubic_ease_in_timing =
      CubicBezierInterpolator::CreatePreset(CubicBezierInterpolator::EaseIn);
  EXPECT_NEAR(0.093f, cubic_ease_in_timing->Interpolate(0.25), tolerance);
  EXPECT_NEAR(0.315f, cubic_ease_in_timing->Interpolate(0.50), tolerance);
  EXPECT_NEAR(0.620f, cubic_ease_in_timing->Interpolate(0.75), tolerance);

  std::unique_ptr<Interpolator> cubic_ease_out_timing =
      CubicBezierInterpolator::CreatePreset(CubicBezierInterpolator::EaseOut);
  EXPECT_NEAR(0.379f, cubic_ease_out_timing->Interpolate(0.25), tolerance);
  EXPECT_NEAR(0.694f, cubic_ease_out_timing->Interpolate(0.50), tolerance);
  EXPECT_NEAR(0.906f, cubic_ease_out_timing->Interpolate(0.75), tolerance);

  std::unique_ptr<Interpolator> cubic_ease_in_out_timing =
      CubicBezierInterpolator::CreatePreset(CubicBezierInterpolator::EaseInOut);
  EXPECT_NEAR(0.128f, cubic_ease_in_out_timing->Interpolate(0.25), tolerance);
  EXPECT_NEAR(0.500f, cubic_ease_in_out_timing->Interpolate(0.50), tolerance);
  EXPECT_NEAR(0.871f, cubic_ease_in_out_timing->Interpolate(0.75), tolerance);
}

}  // namespace testing
}  // namespace clay
