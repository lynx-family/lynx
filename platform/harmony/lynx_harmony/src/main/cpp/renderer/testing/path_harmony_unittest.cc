// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/path_harmony.h"

#include <array>
#include <limits>
#include <vector>

#include "gfx/public/path.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm {
namespace {
using gfx::Path;
using gfx::PathBuilder;
using gfx::PathFillRule;
using Status = PathHarmonyStatus;
class PathHarmonyTest : public ::testing::Test {
 protected:
  void SetUp() override { testing::GetDrawingState() = {}; }
  void TearDown() override {
    EXPECT_EQ(testing::GetDrawingState().live, 0);
    EXPECT_EQ(testing::GetDrawingState().auxiliary_live, 0);
  }
  DrawingPathResult Get(const Path& path, float density = 1,
                        PathFillRule rule = PathFillRule::kNonzero) {
    return PathHarmony::GetDrawingPath(path, rule, density);
  }
  PathHarmonyFactory factory;
};

TEST_F(PathHarmonyTest, NodeDescriptorsReuseDensityAndCacheFailures) {
  auto path = Path::MakeRect(20, 30, factory);
  const auto first = PathHarmony::GetNodeClip(*path, PathFillRule::kNonzero, 2);
  ASSERT_EQ(first.status, HarmonyNodePathStatus::kReady);
  EXPECT_EQ(
      first.descriptor,
      PathHarmony::GetNodeClip(*path, PathFillRule::kNonzero, 2).descriptor);
  EXPECT_NE(
      first.descriptor,
      PathHarmony::GetNodeClip(*path, PathFillRule::kNonzero, 3).descriptor);
  EXPECT_EQ(first.descriptor->MakePayload(6, 8, 200, 200).commands,
            "M6 8L46 8L46 68L6 68Z");
  auto invalid = Path::MakeSvgPathData(base::String("M0 0L1"), 1, factory);
  const auto failure =
      PathHarmony::GetNodeClip(*invalid, PathFillRule::kNonzero, 1);
  EXPECT_EQ(failure.status, HarmonyNodePathStatus::kInvalid);
  EXPECT_EQ(
      failure.descriptor,
      PathHarmony::GetNodeClip(*invalid, PathFillRule::kNonzero, 1).descriptor);
  EXPECT_EQ(testing::GetDrawingState().creates, 0);
}

TEST_F(PathHarmonyTest, FillAndDensityIsolation) {
  auto path = Path::MakeRect(10, 20, factory);
  auto first = Get(*path);
  auto scaled = Get(*path, 2);
  auto parity = Get(*path, 1, PathFillRule::kEvenOdd);
  ASSERT_TRUE(first.path && scaled.path && parity.path);
  EXPECT_NE(first.path, scaled.path);
  EXPECT_NE(first.path, parity.path);
  EXPECT_EQ(first.path->points, (std::vector<float>{0, 0, 10, 20}));
  EXPECT_EQ(scaled.path->points, (std::vector<float>{0, 0, 20, 40}));
  EXPECT_EQ(first.path->fill, PATH_FILL_TYPE_WINDING);
  EXPECT_EQ(parity.path->fill, PATH_FILL_TYPE_EVEN_ODD);
  EXPECT_EQ(Get(*path).path, first.path);
  EXPECT_EQ(testing::GetDrawingState().creates, 3);
}

TEST_F(PathHarmonyTest, SvgScaleOnce) {
  auto path = Path::MakeSvgPathData("M1 2L3 4L5 6Z", 3, factory);
  const auto key = path->GetKey();
  EXPECT_EQ(testing::GetDrawingState().parses, 0);
  auto result = Get(*path, 2);
  ASSERT_EQ(result.status, Status::kReady);
  EXPECT_EQ(result.path->points, (std::vector<float>{6, 12, 18, 24, 30, 36}));
  EXPECT_EQ(Get(*path, 2).path, result.path);
  EXPECT_EQ(testing::GetDrawingState().parses, 1);
  EXPECT_EQ(path->GetKey(), key);
  auto source = std::get<gfx::SvgPathView>(path->GetSourceView());
  EXPECT_EQ(source.data, "M1 2L3 4L5 6Z");
  EXPECT_FLOAT_EQ(source.source_to_layout_scale, 3);
}

TEST_F(PathHarmonyTest, InvalidSvg) {
  auto path = Path::MakeSvgPathData("M1 2L", 1, factory);
  ASSERT_TRUE(path);
  EXPECT_EQ(Get(*path).status, Status::kInvalid);
  EXPECT_EQ(Get(*path).path, nullptr);
  EXPECT_EQ(testing::GetDrawingState().parses, 1);
  EXPECT_EQ(testing::GetDrawingState().creates, 1);
  EXPECT_EQ(testing::GetDrawingState().destroys, 1);
}

TEST_F(PathHarmonyTest, NativeFailureAndDisposal) {
  for (int fail = 0; fail < 3; ++fail) {
    const float radii[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto path = Path::MakeRoundedRect(50, 60, radii, factory);
    testing::GetDrawingState().fail_allocation = fail;
    EXPECT_EQ(Get(*path).status, Status::kInvalid);
    const int creates = testing::GetDrawingState().creates;
    testing::GetDrawingState().fail_allocation = -1;
    EXPECT_EQ(Get(*path).status, Status::kInvalid);
    EXPECT_EQ(testing::GetDrawingState().creates, creates);
    EXPECT_EQ(testing::GetDrawingState().live, 0);
    EXPECT_EQ(testing::GetDrawingState().auxiliary_live, 0);
  }
  const int destroys = testing::GetDrawingState().destroys;
  {
    auto path = Path::MakeRect(10, 20, factory);
    ASSERT_EQ(Get(*path).status, Status::kReady);
    ASSERT_EQ(Get(*path, 2).status, Status::kReady);
    EXPECT_EQ(testing::GetDrawingState().live, 2);
  }
  EXPECT_EQ(testing::GetDrawingState().destroys, destroys + 2);
}

TEST_F(PathHarmonyTest, SvgMatrixFailureAndCoordinateOverflow) {
  auto svg = Path::MakeSvgPathData("M1 2L3 4Z", 1, factory);
  testing::GetDrawingState().fail_allocation = 1;
  EXPECT_EQ(Get(*svg).status, Status::kInvalid);
  EXPECT_EQ(testing::GetDrawingState().live, 0);
  testing::GetDrawingState().fail_allocation = -1;
  EXPECT_EQ(Get(*svg).status, Status::kInvalid);
  EXPECT_EQ(testing::GetDrawingState().parses, 1);
  EXPECT_EQ(Get(*svg, 2).status, Status::kReady);
  auto rect = Path::MakeRect(std::numeric_limits<float>::max(), 10, factory);
  EXPECT_EQ(Get(*rect, 2).status, Status::kInvalid);
  PathBuilder builder;
  builder.MoveTo(std::numeric_limits<float>::max(), 1);
  builder.LineTo(10, 10);
  auto commands = builder.Build(factory);
  EXPECT_EQ(Get(*commands, 2).status, Status::kInvalid);
}

TEST_F(PathHarmonyTest, SquareCornersAndSourceOnlyConstruction) {
  const float radii[8] = {0, 2, 3, 0, 5, 6, 0, 0};
  auto rounded = Path::MakeRoundedRect(50, 60, radii, factory);
  PathBuilder builder;
  builder.MoveTo(1, 2);
  auto commands = builder.Build(factory);
  auto svg = Path::MakeSvgPathData("invalid", 1, factory);
  ASSERT_TRUE(rounded && commands && svg);
  EXPECT_EQ(testing::GetDrawingState().creates, 0);
  EXPECT_EQ(testing::GetDrawingState().parses, 0);
  const auto result = Get(*rounded, 2);
  ASSERT_EQ(result.status, Status::kReady);
  EXPECT_EQ(result.path->radii,
            (std::array<float, 8>{0, 0, 0, 0, 10, 12, 0, 0}));
}

TEST_F(PathHarmonyTest, EmptyInvalidDensityAndNativeError) {
  auto empty = Path::MakeRect(0, 20, factory);
  EXPECT_EQ(Get(*empty).status, Status::kEmpty);
  EXPECT_EQ(Get(*empty).path, nullptr);
  auto path = Path::MakeRect(10, 20, factory);
  EXPECT_EQ(Get(*path, 0).status, Status::kInvalid);
  EXPECT_EQ(Get(*path, -1).status, Status::kInvalid);
  EXPECT_EQ(Get(*path, std::numeric_limits<float>::infinity()).status,
            Status::kInvalid);
  EXPECT_EQ(Get(*path, std::numeric_limits<float>::quiet_NaN()).status,
            Status::kInvalid);
  EXPECT_EQ(Get(*path, 1, static_cast<PathFillRule>(42)).status,
            Status::kUnsupported);
  testing::GetDrawingState().fail_operation = true;
  EXPECT_EQ(Get(*path).status, Status::kInvalid);
  testing::GetDrawingState().fail_operation = false;
  EXPECT_EQ(Get(*path).status, Status::kInvalid);
  EXPECT_EQ(Get(*path, 2).status, Status::kReady);
}
}  // namespace
}  // namespace lynx::tasm
