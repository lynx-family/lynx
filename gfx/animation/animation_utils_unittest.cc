// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/animation_utils.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace gfx {
namespace {

TEST(ColorInterpolationTest, ExplicitColorSpaces) {
  EXPECT_EQ(0xff808080u, InterpolateColorARGB32(0xff000000, 0xffffffff, 0.5,
                                                ColorInterpolation::kSRGB));
  EXPECT_EQ(0xffbcbcbcu,
            InterpolateColorARGB32(0xff000000, 0xffffffff, 0.5,
                                   ColorInterpolation::kLinearRGB));
  EXPECT_EQ(0xff800080u, InterpolateColorARGB32(0xffff0000, 0xff0000ff, 0.5,
                                                ColorInterpolation::kSRGB));
  EXPECT_EQ(0xffbc00bcu,
            InterpolateColorARGB32(0xffff0000, 0xff0000ff, 0.5,
                                   ColorInterpolation::kLinearRGB));
}

TEST(ColorInterpolationTest, LinearRGBUsesPiecewiseSRGBTransferFunction) {
  // Both endpoints are in the linear segment of the transfer function.
  EXPECT_EQ(0xff050505u,
            InterpolateColorARGB32(0xff000000, 0xff0a0a0a, 0.5,
                                   ColorInterpolation::kLinearRGB));
  // Cross the transfer-function boundary, also on the encoded result.
  EXPECT_EQ(0xff0b0b0bu,
            InterpolateColorARGB32(0xff000000, 0xff141414, 0.5,
                                   ColorInterpolation::kLinearRGB));
  EXPECT_EQ(0xff010101u,
            InterpolateColorARGB32(0xff000000, 0xff141414, 0.05,
                                   ColorInterpolation::kLinearRGB));
}

TEST(ColorInterpolationTest, AutoUsesLinearRGB) {
  EXPECT_EQ(0xffbcbcbcu, InterpolateColorARGB32(0xff000000, 0xffffffff, 0.5,
                                                ColorInterpolation::kAuto));
  EXPECT_EQ(0xffbc00bcu, InterpolateColorARGB32(0xffff0000, 0xff0000ff, 0.5,
                                                ColorInterpolation::kAuto));
  EXPECT_EQ(0xff050505u, InterpolateColorARGB32(0xff000000, 0xff0a0a0a, 0.5,
                                                ColorInterpolation::kAuto));
}

TEST(ColorInterpolationTest, PreservesNontransparentEndpoints) {
  for (auto mode : {ColorInterpolation::kSRGB, ColorInterpolation::kLinearRGB,
                    ColorInterpolation::kAuto}) {
    EXPECT_EQ(0x12345678u,
              InterpolateColorARGB32(0x12345678, 0x9abcdef0, 0.0, mode));
    EXPECT_EQ(0x9abcdef0u,
              InterpolateColorARGB32(0x12345678, 0x9abcdef0, 1.0, mode));
    EXPECT_EQ(0x80808080u,
              InterpolateColorARGB32(0x00808080, 0xff808080, 0.5, mode));
  }
}

TEST(ColorInterpolationTest, TransparentColorsDoNotTintInterpolation) {
  for (auto mode : {ColorInterpolation::kSRGB, ColorInterpolation::kLinearRGB,
                    ColorInterpolation::kAuto}) {
    SCOPED_TRACE(static_cast<int>(mode));
    EXPECT_EQ(0x800000ffu,
              InterpolateColorARGB32(0x00ff0000, 0xff0000ff, 0.5, mode));
    EXPECT_EQ(0x800000ffu,
              InterpolateColorARGB32(0xff0000ff, 0x00ff0000, 0.5, mode));
    EXPECT_EQ(0x80ff0000u,
              InterpolateColorARGB32(0xffff0000, 0x00000000, 0.5, mode));
    EXPECT_EQ(0x80ff0000u,
              InterpolateColorARGB32(0x00000000, 0xffff0000, 0.5, mode));
  }
}

TEST(ColorInterpolationTest, PremultipliesInSelectedColorSpace) {
  // Alpha values 64 and 192 give RGB weights of 1/4 and 3/4 at the midpoint.
  EXPECT_EQ(0x804000bfu, InterpolateColorARGB32(0x40ff0000, 0xc00000ff, 0.5,
                                                ColorInterpolation::kSRGB));
  for (auto mode :
       {ColorInterpolation::kLinearRGB, ColorInterpolation::kAuto}) {
    EXPECT_EQ(0x808900e1u,
              InterpolateColorARGB32(0x40ff0000, 0xc00000ff, 0.5, mode));
  }
  // Equal alpha values preserve the unpremultiplied color interpolation.
  EXPECT_EQ(0x80808080u, InterpolateColorARGB32(0x80000000, 0x80ffffff, 0.5,
                                                ColorInterpolation::kSRGB));
  EXPECT_EQ(0x80bcbcbcu,
            InterpolateColorARGB32(0x80000000, 0x80ffffff, 0.5,
                                   ColorInterpolation::kLinearRGB));
}

TEST(ColorInterpolationTest, HandlesZeroAndSmallAlpha) {
  for (auto mode : {ColorInterpolation::kSRGB, ColorInterpolation::kLinearRGB,
                    ColorInterpolation::kAuto}) {
    for (double progress : {0.0, 0.5, 1.0}) {
      EXPECT_EQ(0u,
                InterpolateColorARGB32(0x00ff0000, 0x000000ff, progress, mode));
    }
    // Unpremultiply with the interpolated alpha before rounding it to a byte.
    EXPECT_EQ(0x01ff0000u,
              InterpolateColorARGB32(0x01ff0000, 0x000000ff, 0.5, mode));
    EXPECT_EQ(0u, InterpolateColorARGB32(0x00000000, 0xffff0000, -0.5, mode));
    EXPECT_EQ(0xffff0000u,
              InterpolateColorARGB32(0x00000000, 0xffff0000, 1.5, mode));
  }
}

TEST(ColorInterpolationTest, ExplicitModesClampExtrapolatedChannels) {
  for (auto mode :
       {ColorInterpolation::kSRGB, ColorInterpolation::kLinearRGB}) {
    EXPECT_EQ(0xff000000u,
              InterpolateColorARGB32(0xff000000, 0xffffffff, -0.5, mode));
    EXPECT_EQ(0xffffffffu,
              InterpolateColorARGB32(0xff000000, 0xffffffff, 1.5, mode));
  }
}

}  // namespace
}  // namespace gfx
}  // namespace lynx
