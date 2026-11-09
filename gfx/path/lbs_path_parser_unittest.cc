// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <string>
#include <vector>

#include "gfx/public/path_parser.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace {
struct Segment {
  char type;
  std::array<float, 9> args;
};
std::vector<Segment> Parse(const char* data, bool* error) {
  std::vector<Segment> result;
  LBSPathConsumer c{};
  c.ctx = &result;
  c.MoveToPoint = [](void* p, float x, float y) {
    static_cast<std::vector<Segment>*>(p)->push_back({'M', {x, y}});
  };
  c.LineToPoint = [](void* p, float x, float y) {
    static_cast<std::vector<Segment>*>(p)->push_back({'L', {x, y}});
  };
  c.CubicToPoint = [](void* p, float a, float b, float c, float d, float x,
                      float y) {
    static_cast<std::vector<Segment>*>(p)->push_back({'C', {a, b, c, d, x, y}});
  };
  c.QuadToPoint = [](void* p, float a, float b, float x, float y) {
    static_cast<std::vector<Segment>*>(p)->push_back({'Q', {a, b, x, y}});
  };
  c.EllipticToPoint = [](void* p, float sx, float sy, float rx, float ry,
                         float angle, bool large, bool sweep, float x,
                         float y) {
    static_cast<std::vector<Segment>*>(p)->push_back(
        {'A', {sx, sy, rx, ry, angle, float(large), float(sweep), x, y}});
  };
  c.ClosePath = [](void* p) {
    static_cast<std::vector<Segment>*>(p)->push_back({'Z', {}});
  };
  LBSParsePathWithConsumer(data, &c);
  *error = c.error;
  return result;
}
TEST(LBSPathParserTest, ScientificNotationAndAdjacentNumbers) {
  bool error;
  auto p = Parse("M1e2 3 l-2.5+.5 1E-1-3.4e+1", &error);
  ASSERT_FALSE(error);
  ASSERT_EQ(p.size(), 3u);
  EXPECT_EQ(p[0].args[0], 100);
  EXPECT_EQ(p[0].args[1], 3);
  EXPECT_EQ(p[1].args[0], 97.5);
  EXPECT_EQ(p[1].args[1], 3.5);
  EXPECT_NEAR(p[2].args[0], 97.6, 1e-4);
  EXPECT_EQ(p[2].args[1], -30.5);
}
TEST(LBSPathParserTest, SmoothControlUsesPreviousCommand) {
  bool error;
  auto p =
      Parse("M0 0 L10 0 S20 10 30 0 s10 10 20 0 Q50 10 60 0 T70 0 L80 0 t10 0",
            &error);
  ASSERT_FALSE(error);
  ASSERT_EQ(p.size(), 8u);
  EXPECT_EQ(p[2].args[0], 10);
  EXPECT_EQ(p[2].args[1], 0);
  EXPECT_EQ(p[3].args[0], 40);
  EXPECT_EQ(p[3].args[1], -10);
  EXPECT_EQ(p[5].args[0], 70);
  EXPECT_EQ(p[5].args[1], -10);
  EXPECT_EQ(p[7].args[0], 80);
  EXPECT_EQ(p[7].args[1], 0);
}
TEST(LBSPathParserTest, CloseRestoresSubpathStart) {
  bool error;
  auto p = Parse("M10 10 L20 20 Z l5 0 m1 2 3 4 z", &error);
  ASSERT_FALSE(error);
  ASSERT_EQ(p.size(), 7u);
  EXPECT_EQ(p[3].args[0], 15);
  EXPECT_EQ(p[3].args[1], 10);
  EXPECT_EQ(p[4].args[0], 16);
  EXPECT_EQ(p[4].args[1], 12);
  EXPECT_EQ(p[5].type, 'L');
  EXPECT_EQ(p[5].args[0], 19);
}
TEST(LBSPathParserTest, SmoothControlAcrossCurveFamiliesAndClose) {
  struct TestCase {
    const char* path;
    float x;
    float y;
  };
  for (const auto& test :
       {TestCase{"M10 20 S30 40 50 60", 10, 20},
        TestCase{"M10 20 Q11 22 30 40 S50 60 70 80", 30, 40},
        TestCase{"M10 20 C11 22 33 44 50 60 T70 80", 50, 60},
        TestCase{"M10 20 q1 2 3 4 s5 6 7 8", 13, 24},
        TestCase{"M10 20 c1 2 3 4 5 6 t7 8", 15, 26},
        TestCase{"M10 20 C11 22 33 44 50 60 Z S70 80 90 100", 10, 20},
        TestCase{"M10 20 Q11 22 30 40 Z T50 60", 10, 20}}) {
    bool error;
    auto p = Parse(test.path, &error);
    ASSERT_FALSE(error) << test.path;
    ASSERT_FALSE(p.empty());
    EXPECT_EQ(p.back().args[0], test.x) << test.path;
    EXPECT_EQ(p.back().args[1], test.y) << test.path;
  }
}
TEST(LBSPathParserTest, ImplicitSmoothCurvesRetainControls) {
  bool error;
  auto p = Parse("M0 0 C1 2 3 4 5 6 S7 8 9 10 11 12 13 14", &error);
  ASSERT_FALSE(error);
  ASSERT_EQ(p.size(), 4u);
  EXPECT_EQ(p[2].args[0], 7);
  EXPECT_EQ(p[2].args[1], 8);
  EXPECT_EQ(p[3].args[0], 11);
  EXPECT_EQ(p[3].args[1], 12);
  p = Parse("M0 0 Q1 2 3 4 T5 6 7 8", &error);
  ASSERT_FALSE(error);
  ASSERT_EQ(p.size(), 4u);
  EXPECT_EQ(p[2].args[0], 5);
  EXPECT_EQ(p[2].args[1], 6);
  EXPECT_EQ(p[3].args[0], 5);
  EXPECT_EQ(p[3].args[1], 6);
}
TEST(LBSPathParserTest, SmoothReflectionAvoidsIntermediateOverflow) {
  bool error;
  auto p = Parse(
      "M3e38 3e38 C3e38 3e38 3e38 3e38 3e38 3e38 "
      "S3e38 3e38 3e38 3e38",
      &error);
  ASSERT_FALSE(error);
  ASSERT_EQ(p.size(), 3u);
  EXPECT_EQ(p.back().args[0], p.front().args[0]);
  EXPECT_EQ(p.back().args[1], p.front().args[1]);
}
TEST(LBSPathParserTest, AllCommandsAndPackedArcFlags) {
  bool error;
  Parse(
      "M0 0 H2 h2 V2 v2 L1 1 l1 1 C1 2 3 4 5 6 c1 2 3 4 5 6 S1 2 3 4 s1 2 3 4 "
      "Q1 2 3 4 q1 2 3 4 T1 2 t1 2 A1 2 30 01-2 3 a1 2 30 1 0 2 3 Z m1 1 z",
      &error);
  EXPECT_FALSE(error);
  auto p = Parse("M1 2a3 4 45 10 5 6", &error);
  ASSERT_FALSE(error);
  ASSERT_EQ(p.size(), 2u);
  EXPECT_EQ(p[1].args[4], 45);
  EXPECT_EQ(p[1].args[5], 1);
  EXPECT_EQ(p[1].args[6], 0);
  EXPECT_EQ(p[1].args[7], 6);
  EXPECT_EQ(p[1].args[8], 8);
}
TEST(LBSPathParserTest, IncompleteOrOverflowingArgumentsStopBeforeCallback) {
  for (const char* data : {"M",
                           "M1",
                           "M1e 2",
                           "M. 2",
                           "M0 0 L3",
                           "M0 0 H",
                           "M0 0 V",
                           "M0 0 C1 2 3",
                           "M0 0 S1 2",
                           "M0 0 Q1",
                           "M0 0 T",
                           "M0 0 A1",
                           "M0 0 A1 1 0",
                           "M0 0 A1 1 0 0",
                           "M0 0 A1 1 0 0 1",
                           "M0 0 A1 1 0 0 1 2",
                           "M0 0 L1e40 2",
                           "M3e38 0 l3e38 0",
                           "M0 0 Z1",
                           "M0 0 \x7f",
                           "M0 0 \xff",
                           "M0 0 L\xc2\xa0 1"}) {
    bool error;
    auto p = Parse(data, &error);
    EXPECT_TRUE(error) << data;
    EXPECT_LE(p.size(), std::string(data) == "M0 0 Z1" ? 2u : 1u) << data;
  }
  bool error;
  Parse(nullptr, &error);
  EXPECT_TRUE(error);
  Parse(" \n\t", &error);
  EXPECT_FALSE(error);
}
TEST(LBSPathParserTest, RetainsPermissiveSeparatorsAndNoLengthLimit) {
  for (const char* data : {"L1 2", "M0 0,", "M0 0 L,1,,1"}) {
    bool error;
    Parse(data, &error);
    EXPECT_FALSE(error) << data;
  }
  bool error;
  auto p = Parse(std::string((1u << 20) + 1, ' ').c_str(), &error);
  EXPECT_FALSE(error);
  EXPECT_TRUE(p.empty());
}
}  // namespace
