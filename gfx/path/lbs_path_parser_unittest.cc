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
}  // namespace
