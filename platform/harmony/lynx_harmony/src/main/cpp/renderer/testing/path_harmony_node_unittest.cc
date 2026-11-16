// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/path_harmony_node.h"

#include <limits>
#include <string>

#include "gfx/public/path.h"
#include "gfx/testing/path_stream_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm {
namespace {
const auto& factory = gfx::GetTestPathImplFactory();

HarmonyNodePathDescriptor Describe(const gfx::Path& path, float density = 1) {
  return HarmonyNodePathDescriptor::Build(path.GetSourceView(),
                                          gfx::PathFillRule::kNonzero, density);
}

TEST(HarmonyNodePathTest, CommandsAndRadii) {
  const float radii[] = {1, 2, 3, 4, 5, 6, 7, 8};
  const auto rounded = gfx::Path::MakeRoundedRect(30, 40, radii, factory);
  const auto descriptor = Describe(*rounded, 2);
  ASSERT_EQ(descriptor.status(), HarmonyNodePathStatus::kReady);
  EXPECT_EQ(descriptor.MakePayload(0, 0, 60, 80).commands,
            "M2 0L54 0A6 8 0 0 1 60 8L60 68A10 12 0 0 1 50 80L14 80"
            "A14 16 0 0 1 0 64L0 4A2 4 0 0 1 2 0Z");
  gfx::PathBuilder builder;
  builder.MoveTo(1, 2);
  builder.LineTo(3, 4);
  builder.QuadTo(5, 6, 7, 8);
  builder.CubicTo(9, 10, 11, 12, 13, 14);
  builder.Close();
  EXPECT_EQ(
      Describe(*builder.Build(factory), 2).MakePayload(0, 0, 100, 100).commands,
      "M2 4L6 8Q10 12 14 16C18 20 22 24 26 28Z");
}

TEST(HarmonyNodePathTest, SvgScaleAndPlacement) {
  auto path = gfx::Path::MakeSvgPathData(
      base::String("m1 2 l3 4 q1 2 3 4 c1 2 3 4 5 6 z"), 2, factory);
  const auto descriptor = Describe(*path, 3);
  const auto canonical = descriptor.MakePayload(0, 0, 300, 300);
  EXPECT_EQ(canonical.commands, "M6 12L24 36Q30 48 42 60C48 72 60 84 72 96Z");
  EXPECT_EQ(descriptor.MakePayload(10, 20, 300, 300).commands,
            "M16 32L34 56Q40 68 52 80C58 92 70 104 82 116Z");
  EXPECT_EQ(descriptor.MakePayload(-10, -20, 300, 300).commands,
            "M-4 -8L14 16Q20 28 32 40C38 52 50 64 62 76Z");
  EXPECT_EQ(descriptor.MakePayload(0, 0, 300, 300).commands,
            canonical.commands);
  EXPECT_FLOAT_EQ(canonical.width, 300);
  EXPECT_FLOAT_EQ(canonical.height, 300);
  EXPECT_EQ(std::get<gfx::SvgPathView>(path->GetSourceView()).data,
            "m1 2 l3 4 q1 2 3 4 c1 2 3 4 5 6 z");
}

TEST(HarmonyNodePathTest, SvgArcsPreserveAnglesFlagsAndRelativeEndpoints) {
  const auto path = gfx::Path::MakeSvgPathData(
      base::String("M1 2 a3 4 45 1 0 5 6 A7 8 30 0 1 9 10"), 2, factory);
  const auto descriptor = Describe(*path, 3);
  ASSERT_EQ(descriptor.status(), HarmonyNodePathStatus::kReady);
  EXPECT_EQ(descriptor.MakePayload(10, 20, 300, 300).commands,
            "M16 32A18 24 45 1 0 46 68A42 48 30 0 1 64 80");
}

TEST(HarmonyNodePathTest, EndDoesNotCloseSubpaths) {
  gfx::PathBuilder builder;
  builder.MoveTo(1, 2);
  builder.LineTo(3, 4);
  builder.MoveTo(5, 6);
  builder.LineTo(7, 8);
  const auto path = builder.Build(factory);
  EXPECT_EQ(Describe(*path).MakePayload(0, 0, 20, 20).commands,
            "M1 2L3 4M5 6L7 8");
}

TEST(HarmonyNodePathTest, EvenOddUnsupported) {
  const auto path = gfx::Path::MakeRect(10, 10, factory);
  const auto descriptor = HarmonyNodePathDescriptor::Build(
      path->GetSourceView(), gfx::PathFillRule::kEvenOdd, 1);
  EXPECT_EQ(descriptor.status(), HarmonyNodePathStatus::kUnsupported);
  const auto payload = descriptor.MakePayload(0, 0, 20, 20);
  EXPECT_EQ(payload.status, HarmonyNodePathStatus::kUnsupported);
  EXPECT_TRUE(payload.commands.empty());
}

TEST(HarmonyNodePathTest, EmptyIsNotReset) {
  const auto path = gfx::Path::MakeRect(0, 10, factory);
  const auto descriptor = Describe(*path);
  EXPECT_EQ(descriptor.status(), HarmonyNodePathStatus::kEmpty);
  const auto payload = descriptor.MakePayload(0, 0, 20, 20);
  EXPECT_EQ(payload.status, HarmonyNodePathStatus::kEmpty);
  EXPECT_GT(payload.width, 0);
  EXPECT_GT(payload.height, 0);
  EXPECT_EQ(descriptor.MakePayload(0, 0, 0, 20).status,
            HarmonyNodePathStatus::kInvalid);
  gfx::PathBuilder builder;
  const auto empty = builder.Build(factory);
  EXPECT_EQ(Describe(*empty).MakePayload(0, 0, 20, 20).status,
            HarmonyNodePathStatus::kEmpty);
  const auto move_only =
      gfx::Path::MakeSvgPathData(base::String("M1 2Z"), 1, factory);
  EXPECT_EQ(Describe(*move_only).status(), HarmonyNodePathStatus::kEmpty);
}

TEST(HarmonyNodePathTest, InvalidSvg) {
  for (const char* data : {"M0 0L1", "M0 0L1 2garbage", "M0 0A1 2 30 0",
                           "M0 0L1e999 0", "L10 10", "Z"}) {
    const auto path =
        gfx::Path::MakeSvgPathData(base::String(data), 1, factory);
    ASSERT_NE(path, nullptr);
    const auto descriptor = Describe(*path);
    EXPECT_EQ(descriptor.status(), HarmonyNodePathStatus::kInvalid) << data;
    EXPECT_TRUE(descriptor.MakePayload(0, 0, 20, 20).commands.empty());
  }
}

TEST(HarmonyNodePathTest, RejectsNonFiniteScaleGeometryAndPlacement) {
  const auto path = gfx::Path::MakeRect(10, 10, factory);
  const auto infinity = std::numeric_limits<float>::infinity();
  for (float density :
       {0.f, -1.f, infinity, std::numeric_limits<float>::quiet_NaN()}) {
    EXPECT_EQ(Describe(*path, density).status(),
              HarmonyNodePathStatus::kInvalid);
  }
  const auto large =
      gfx::Path::MakeRect(std::numeric_limits<float>::max(), 1, factory);
  EXPECT_EQ(Describe(*large, 2).status(), HarmonyNodePathStatus::kInvalid);
  const auto descriptor = Describe(*path);
  EXPECT_EQ(descriptor.MakePayload(infinity, 0, 20, 20).status,
            HarmonyNodePathStatus::kInvalid);
  EXPECT_EQ(descriptor.MakePayload(0, 0, infinity, 20).status,
            HarmonyNodePathStatus::kInvalid);
  const auto svg = gfx::Path::MakeSvgPathData(
      base::String("M0 0L1 1"), std::numeric_limits<float>::max(), factory);
  EXPECT_EQ(Describe(*svg, 2).status(), HarmonyNodePathStatus::kInvalid);
}

}  // namespace
}  // namespace lynx::tasm
