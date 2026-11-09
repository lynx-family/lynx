// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/path.h"

#include <cmath>
#include <limits>
#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx::gfx {
TEST(PathTest, ImmutableSourceAndIdentity) {
  auto a = Path::MakeRect(10, 20);
  auto b = Path::MakeRect(10, 20);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  EXPECT_NE(a->GetKey(), 0u);
  EXPECT_NE(a->GetKey(), b->GetKey());
  EXPECT_EQ(a->GetKey(), a->GetImpl().GetKey());
  auto view = std::get<PathView>(a->GetSourceView());
  EXPECT_EQ(view.header.width, 10);
  EXPECT_EQ(view.header.height, 20);
  PathImpl impl(a->GetImpl().GetSource());
  EXPECT_NE(impl.GetKey(), a->GetKey());
  EXPECT_EQ(std::get<PathView>(impl.GetSourceView()).header.width, 10);
}
TEST(PathTest, MovedSourceRemainsAValidEmptyValue) {
  auto path = Path::MakeSvgPathData("M0 0L1 1", 1);
  auto source = path->GetImpl().GetSource();
  PathImpl impl(std::move(source));
  EXPECT_EQ(std::get<SvgPathView>(impl.GetSourceView()).data, "M0 0L1 1");
  PathImpl moved_from(std::move(source));
  EXPECT_EQ(std::get<PathView>(moved_from.GetSourceView()).header.width, 0);
  EXPECT_EQ(std::get<PathView>(moved_from.GetSourceView()).header.command_count,
            0u);
}

TEST(PathTest, RoundedRectNormalization) {
  float radii[8] = {100, 50, 100, 50, 10, 10, 10, 10};
  auto path = Path::MakeRoundedRect(100, 100, radii);
  ASSERT_TRUE(path);
  const auto& h = std::get<PathView>(path->GetSourceView()).header;
  EXPECT_EQ(h.radii[0], 50);
  EXPECT_EQ(h.radii[1], 25);
  EXPECT_EQ(h.radii[4], 5);
}
TEST(PathTest, InvalidConstruction) {
  float inf = std::numeric_limits<float>::infinity();
  float radii[8] = {};
  radii[0] = -1;
  EXPECT_FALSE(Path::MakeRect(inf, 1));
  EXPECT_FALSE(Path::MakeRect(-1, 1));
  EXPECT_FALSE(Path::MakeRoundedRect(10, 10, radii));
  PathBuilder builder;
  builder.MoveTo(inf, 0);
  EXPECT_FALSE(builder.Build());
  EXPECT_TRUE(Path::MakeRect(0, 0));
}
TEST(PathTest, BuilderPublicationAndEnd) {
  PathBuilder b;
  b.MoveTo(1, 2);
  b.LineTo(3, 4);
  auto a = b.Build();
  ASSERT_TRUE(a);
  auto view = std::get<PathView>(a->GetSourceView());
  EXPECT_EQ(view.header.command_count, 3u);
  EXPECT_EQ(view.commands[2], PathVerb::kEnd);
  b.MoveTo(5, 6);
  b.QuadTo(7, 8, 9, 10);
  b.CubicTo(1, 2, 3, 4, 5, 6);
  b.Close();
  auto other = b.Build();
  ASSERT_TRUE(other);
  EXPECT_EQ(view.points[0].x, 1);
  EXPECT_EQ(view.points[1].y, 4);
  EXPECT_EQ(std::get<PathView>(other->GetSourceView()).commands[3],
            PathVerb::kClose);
  b.LineTo(0, 0);
  EXPECT_FALSE(b.Build());
  b.MoveTo(1, 1);
  EXPECT_TRUE(b.Build());
}
TEST(PathTest, BuiltCommandsAndCopiedSourceOwnTheirBytes) {
  PathPoint points[] = {{1, 2}, {-30, 40}, {50, -60}, {3, 4}};
  PathBuilder builder(3, 4);
  builder.MoveTo(points[0].x, points[0].y);
  builder.CubicTo(points[1].x, points[1].y, points[2].x, points[2].y,
                  points[3].x, points[3].y);
  auto path = builder.Build();
  ASSERT_TRUE(path);
  auto source = path->GetImpl().GetSource();
  PathImpl copy(source);
  points[1] = {0, 0};
  auto original = std::get<PathView>(path->GetSourceView());
  auto copied = std::get<PathView>(copy.GetSourceView());
  EXPECT_EQ(original.commands[1], PathVerb::kCubic);
  EXPECT_EQ(copied.commands[1], PathVerb::kCubic);
  EXPECT_EQ(copied.points[1].x, -30);
  EXPECT_EQ(original.points, copied.points);
  path = nullptr;
  source = Path::MakeSvgPathData("M0 0", 1)->GetImpl().GetSource();
  EXPECT_EQ(copied.points[2].x, 50);
  PathImpl moved(std::move(source));
  EXPECT_EQ(std::get<SvgPathView>(moved.GetSourceView()).data, "M0 0");
  EXPECT_EQ(std::get<PathView>(source.GetSourceView()).header.point_count, 0u);
}
TEST(PathTest, BuilderGrowthPreservesSeparateCommandAndPointRegions) {
  const size_t capacities[][2] = {{0, 0}, {1, 1}, {1, 4096}, {4096, 1}};
  for (const auto& capacity : capacities) {
    PathBuilder builder(capacity[0], capacity[1]);
    builder.MoveTo(-1, -2);
    for (int i = 0; i < 1000; ++i)
      builder.CubicTo(i, -i, i + 1, -i - 1, i + 2, -i - 2);
    builder.Close();
    auto retained = builder.Build();
    ASSERT_TRUE(retained);
    auto view = std::get<PathView>(retained->GetSourceView());
    EXPECT_EQ(view.header.command_count, 1002u);
    EXPECT_EQ(view.header.point_count, 3001u);
    for (int i = 0; i < 1000; ++i) {
      EXPECT_EQ(view.commands[i + 1], PathVerb::kCubic);
      EXPECT_EQ(view.points[3 * i + 1].x, i);
      EXPECT_EQ(view.points[3 * i + 3].y, -i - 2);
    }
    builder.MoveTo(7, 8);
    ASSERT_TRUE(builder.Build());
    EXPECT_EQ(view.points[3000].x, 1001);
  }
  PathBuilder oversized(0, Path::kMaxPoints + 1);
  EXPECT_FALSE(oversized.Build());
  oversized.MoveTo(1, 2);
  EXPECT_TRUE(oversized.Build());
}
TEST(PathTest, CloseAtCapacityPreservesPoint) {
  PathBuilder builder(1, 1);
  builder.MoveTo(-4, 7);
  builder.Close();
  auto path = builder.Build();
  ASSERT_TRUE(path);
  auto view = std::get<PathView>(path->GetSourceView());
  EXPECT_EQ(view.header.command_count, 2u);
  EXPECT_EQ(view.header.point_count, 1u);
  EXPECT_EQ(view.commands[1], PathVerb::kClose);
  EXPECT_EQ(view.points[0].x, -4);
  EXPECT_EQ(view.points[0].y, 7);
}
TEST(PathTest, CurveWithNaNRejectsWholeBuildAndAllowsReuse) {
  PathBuilder builder(1, 1);
  builder.MoveTo(1, 2);
  builder.CubicTo(-10, -20, std::numeric_limits<float>::quiet_NaN(), 30, 40,
                  50);
  builder.LineTo(60, 70);
  EXPECT_FALSE(builder.Build());
  builder.MoveTo(3, 4);
  builder.LineTo(5, 6);
  auto path = builder.Build();
  ASSERT_TRUE(path);
  auto view = std::get<PathView>(path->GetSourceView());
  EXPECT_EQ(view.header.command_count, 3u);
  EXPECT_EQ(view.header.point_count, 2u);
}
TEST(PathTest, CommandGeometryHasNoRecordedBounds) {
  PathBuilder builder;
  const float limit = std::numeric_limits<float>::max();
  builder.MoveTo(-limit, -limit);
  builder.LineTo(limit, limit);
  auto path = builder.Build();
  ASSERT_TRUE(path);
  auto view = std::get<PathView>(path->GetSourceView());
  EXPECT_EQ(view.points[0].x, -limit);
  EXPECT_EQ(view.points[1].y, limit);
  EXPECT_EQ(view.header.x, 0);
  EXPECT_EQ(view.header.y, 0);
  EXPECT_EQ(view.header.width, 0);
  EXPECT_EQ(view.header.height, 0);
}
TEST(PathTest, DrawingMethodsRejectNaNAndInfinity) {
  const float invalid[] = {std::numeric_limits<float>::quiet_NaN(),
                           std::numeric_limits<float>::infinity(),
                           -std::numeric_limits<float>::infinity()};
  for (float value : invalid) {
    PathBuilder builder;
    for (int i = 0; i < 6; ++i) {
      float p[] = {1, 2, 3, 4, 5, 6};
      p[i] = value;
      if (i < 2) {
        builder.MoveTo(p[0], p[1]);
        EXPECT_FALSE(builder.Build());
        builder.MoveTo(0, 0);
        builder.LineTo(p[0], p[1]);
        EXPECT_FALSE(builder.Build());
      }
      if (i < 4) {
        builder.MoveTo(0, 0);
        builder.QuadTo(p[0], p[1], p[2], p[3]);
        EXPECT_FALSE(builder.Build());
      }
      builder.MoveTo(0, 0);
      builder.CubicTo(p[0], p[1], p[2], p[3], p[4], p[5]);
      EXPECT_FALSE(builder.Build());
    }
    builder.MoveTo(7, 8);
    ASSERT_TRUE(builder.Build());
  }
}
TEST(PathTest, ParserStopsRelativeCoordinateOverflowBeforeDrawing) {
  PathBuilder builder;
  auto consumer = builder.GetConsumer();
  LBSParsePathWithConsumer("M3e38 0 l3e38 0", &consumer);
  EXPECT_EQ(consumer.error, 1);
  auto path = builder.Build();
  ASSERT_TRUE(path);
  const auto view = std::get<PathView>(path->GetSourceView());
  EXPECT_EQ(view.header.point_count, 1u);
  EXPECT_FLOAT_EQ(view.points[0].x, 3e38f);
  EXPECT_EQ(view.header.command_count, 2u);
  EXPECT_EQ(view.commands[1], PathVerb::kEnd);
  builder.MoveTo(1, 2);
  EXPECT_TRUE(builder.Build());
}
TEST(PathTest, AppendingPastPointLimitRejectsBuildAndAllowsReuse) {
  PathBuilder builder(Path::kMaxPoints + 2, Path::kMaxPoints);
  builder.MoveTo(0, 0);
  for (size_t i = 1; i < Path::kMaxPoints; ++i) builder.LineTo(1, 2);
  builder.LineTo(3, 4);
  EXPECT_FALSE(builder.Build());
  builder.MoveTo(5, 6);
  EXPECT_TRUE(builder.Build());
}
TEST(PathTest, SharedConsumerBuildsCurvesAndRejectsUnsupportedArcs) {
  PathBuilder builder;
  auto consumer = builder.GetConsumer();
  LBSParsePathWithConsumer("M1 2 Q3 4 5 6 C7 8 9 10 11 12 Z M13 14 L15 16",
                           &consumer);
  ASSERT_EQ(consumer.error, 0);
  auto path = builder.Build();
  ASSERT_TRUE(path);
  auto view = std::get<PathView>(path->GetSourceView());
  EXPECT_EQ(view.header.command_count, 7u);
  EXPECT_EQ(view.header.point_count, 8u);
  EXPECT_EQ(view.commands[1], PathVerb::kQuadratic);
  EXPECT_EQ(view.commands[2], PathVerb::kCubic);
  EXPECT_EQ(view.commands[3], PathVerb::kClose);
  EXPECT_EQ(view.commands[6], PathVerb::kEnd);
  EXPECT_EQ(view.points[7].y, 16);
  LBSParsePathWithConsumer("M0 0 A1 2 30 0 1 3 4", &consumer);
  EXPECT_FALSE(builder.Build());
}
TEST(PathTest, ConsumerContinuesDrawingAfterClose) {
  const struct {
    const char* data;
    PathVerb verb;
    uint32_t points;
    PathPoint start;
    PathPoint end;
  } cases[] = {
      {"M0 0 L10 10 Z L20 20", PathVerb::kLine, 4, {0, 0}, {20, 20}},
      {"M3 4 L10 11 Z Q12 13 14 15", PathVerb::kQuadratic, 5, {3, 4}, {14, 15}},
      {"M3 4 L10 11 Z C12 13 14 15 16 17",
       PathVerb::kCubic,
       6,
       {3, 4},
       {16, 17}},
  };
  const size_t capacities[][2] = {{0, 0}, {3, 2}, {1, 4096}, {4096, 1}};
  for (const auto& test : cases) {
    SCOPED_TRACE(test.data);
    for (const auto& capacity : capacities) {
      PathBuilder builder(capacity[0], capacity[1]);
      auto consumer = builder.GetConsumer();
      LBSParsePathWithConsumer(test.data, &consumer);
      ASSERT_EQ(consumer.error, 0);
      auto path = builder.Build();
      ASSERT_TRUE(path);
      auto view = std::get<PathView>(path->GetSourceView());
      const PathVerb expected[] = {PathVerb::kBegin, PathVerb::kLine,
                                   PathVerb::kClose, PathVerb::kBegin,
                                   test.verb,        PathVerb::kEnd};
      ASSERT_EQ(view.header.command_count, 6u);
      ASSERT_EQ(view.header.point_count, test.points);
      for (size_t i = 0; i < 6; ++i) EXPECT_EQ(view.commands[i], expected[i]);
      EXPECT_EQ(view.points[2].x, test.start.x);
      EXPECT_EQ(view.points[2].y, test.start.y);
      EXPECT_EQ(view.points[test.points - 1].x, test.end.x);
      EXPECT_EQ(view.points[test.points - 1].y, test.end.y);
    }
  }
}
TEST(PathTest, ConsumerUsesLatestStartAfterRepeatedClose) {
  PathBuilder builder;
  auto consumer = builder.GetConsumer();
  LBSParsePathWithConsumer("M1 2 L3 4 Z M5 6 L7 8 Z Z L9 10", &consumer);
  ASSERT_EQ(consumer.error, 0);
  auto path = builder.Build();
  ASSERT_TRUE(path);
  auto view = std::get<PathView>(path->GetSourceView());
  ASSERT_EQ(view.header.command_count, 9u);
  ASSERT_EQ(view.header.point_count, 6u);
  EXPECT_EQ(view.commands[6], PathVerb::kBegin);
  EXPECT_EQ(view.points[4].x, 5);
  EXPECT_EQ(view.points[4].y, 6);
  EXPECT_EQ(view.points[5].x, 9);
  EXPECT_EQ(view.points[5].y, 10);
}
TEST(PathTest, ConsumerContinuationSharesStateAndResetsAtBuild) {
  PathBuilder builder;
  auto consumer = builder.GetConsumer();
  builder.MoveTo(5, 6);
  builder.Close();
  consumer.LineToPoint(consumer.ctx, 7, 8);
  auto first = builder.Build();
  ASSERT_TRUE(first);
  auto view = std::get<PathView>(first->GetSourceView());
  ASSERT_EQ(view.header.point_count, 3u);
  EXPECT_EQ(view.points[1].x, 5);
  EXPECT_EQ(view.points[1].y, 6);
  consumer.LineToPoint(consumer.ctx, 9, 10);
  EXPECT_FALSE(builder.Build());
  consumer.MoveToPoint(consumer.ctx, 1, 2);
  consumer.ClosePath(consumer.ctx);
  consumer.QuadToPoint(consumer.ctx, 3, 4, 5, 6);
  ASSERT_TRUE(builder.Build());
  EXPECT_EQ(view.points[2].x, 7);
  EXPECT_EQ(view.points[2].y, 8);
}
TEST(PathTest, DirectDrawingContinuesFromClosedStart) {
  PathBuilder builder;
  builder.MoveTo(3, 4);
  builder.LineTo(5, 6);
  builder.Close();
  builder.Close();
  builder.QuadTo(7, 8, 9, 10);
  auto path = builder.Build();
  ASSERT_TRUE(path);
  auto view = std::get<PathView>(path->GetSourceView());
  ASSERT_EQ(view.header.command_count, 6u);
  ASSERT_EQ(view.header.point_count, 5u);
  EXPECT_EQ(view.commands[3], PathVerb::kBegin);
  EXPECT_EQ(view.points[2].x, 3);
  EXPECT_EQ(view.points[2].y, 4);
}
TEST(PathTest, DirectMethodsAndConsumerShareStateAcrossBuilds) {
  PathBuilder builder;
  auto consumer = builder.GetConsumer();
  builder.MoveTo(1, 2);
  consumer.LineToPoint(consumer.ctx, 3, 4);
  builder.QuadTo(5, 6, 7, 8);
  consumer.CubicToPoint(consumer.ctx, 9, 10, 11, 12, 13, 14);
  builder.Close();
  auto first = builder.Build();
  ASSERT_TRUE(first);
  auto view = std::get<PathView>(first->GetSourceView());
  ASSERT_EQ(view.header.command_count, 5u);
  ASSERT_EQ(view.header.point_count, 7u);
  EXPECT_EQ(view.commands[0], PathVerb::kBegin);
  EXPECT_EQ(view.commands[1], PathVerb::kLine);
  EXPECT_EQ(view.commands[2], PathVerb::kQuadratic);
  EXPECT_EQ(view.commands[3], PathVerb::kCubic);
  EXPECT_EQ(view.commands[4], PathVerb::kClose);
  EXPECT_EQ(view.points[6].x, 13);
  builder.Close();
  EXPECT_FALSE(builder.Build());
  consumer.MoveToPoint(consumer.ctx, 20, 21);
  builder.LineTo(22, 23);
  auto second = builder.Build();
  ASSERT_TRUE(second);
  EXPECT_EQ(std::get<PathView>(second->GetSourceView()).points[0].x, 20);
  EXPECT_EQ(view.points[6].y, 14);
}
TEST(PathTest, EmptyBuilderPublishesEmptyGeometryAndCanBeReused) {
  PathBuilder builder;
  auto empty = builder.Build();
  ASSERT_TRUE(empty);
  auto view = std::get<PathView>(empty->GetSourceView());
  EXPECT_EQ(view.header.kind, PathKind::kCommands);
  EXPECT_EQ(view.header.command_count, 0u);
  EXPECT_EQ(view.header.point_count, 0u);
  EXPECT_EQ(view.commands, nullptr);
  EXPECT_EQ(view.points, nullptr);
  builder.MoveTo(1, 2);
  EXPECT_TRUE(builder.Build());
  EXPECT_EQ(view.header.point_count, 0u);
}
TEST(PathTest, SvgPreservesBytesAndScale) {
  std::string source = "m1e2 3 A10 20 45 01-2.5 .3z\n";
  auto path = Path::MakeSvgPathData(source, 2.f);
  ASSERT_TRUE(path);
  auto a = std::get<SvgPathView>(path->GetSourceView());
  auto b = std::get<SvgPathView>(path->GetSourceView());
  EXPECT_EQ(a.data, source);
  EXPECT_EQ(&a.data, &b.data);
  EXPECT_EQ(a.source_to_layout_scale, 2);
  EXPECT_FALSE(Path::MakeSvgPathData(source, 0));
  EXPECT_FALSE(
      Path::MakeSvgPathData(source, std::numeric_limits<float>::quiet_NaN()));
  EXPECT_FALSE(Path::MakeSvgPathData(std::string("M0 0\0L1 1", 10), 1));
  EXPECT_FALSE(
      Path::MakeSvgPathData(std::string(Path::kMaxSvgBytes + 1, ' '), 1));
  EXPECT_TRUE(Path::MakeSvgPathData("", 1));
}
TEST(PathTest, SvgSourceRetentionDoesNotRequireParsing) {
  for (const char* text : {"M0 0 A1", "M0 0 L1 1 L2", "M0 0 L1e40 2", "L1 2"}) {
    base::String data(text);
    auto path = Path::MakeSvgPathData(data, 1);
    ASSERT_TRUE(path) << text;
    const auto& source = std::get<SvgPathView>(path->GetSourceView()).data;
    EXPECT_EQ(&source, &data.str());
    EXPECT_EQ(source, text);
  }
}
TEST(PathTest, SvgSharesComputedStringAcrossScaleAndSourceCopies) {
  base::String data("M0 0 L1 2 C3 4 5 6 7 8 Z");
  auto first = Path::MakeSvgPathData(data, 1);
  auto second = Path::MakeSvgPathData(data, 2);
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  PathImpl copy(first->GetImpl().GetSource());
  const auto& a = std::get<SvgPathView>(first->GetSourceView()).data;
  const auto& b = std::get<SvgPathView>(second->GetSourceView()).data;
  const auto& c = std::get<SvgPathView>(copy.GetSourceView()).data;
  EXPECT_EQ(&a, &data.str());
  EXPECT_EQ(&b, &data.str());
  EXPECT_EQ(&c, &data.str());
  data = base::String();
  first = nullptr;
  second = nullptr;
  EXPECT_EQ(c, "M0 0 L1 2 C3 4 5 6 7 8 Z");
}
TEST(PathTest, SuperEllipseSamplesMatchLegacyFormula) {
  constexpr double half_pi = 1.57079632679489661923;
  const float exponents[][2] = {{2, 2}, {4, 3}, {0.2f, 2.3f}, {10, 10}};
  for (const auto& exponent : exponents) {
    auto path = Path::MakeSuperEllipse(60, 40, exponent[0], exponent[1]);
    ASSERT_TRUE(path);
    auto view = std::get<PathView>(path->GetSourceView());
    size_t index = 0;
    for (int q = 0; q < 4; ++q) {
      float sx = (q == 0 || q == 3) ? 1.f : -1.f;
      float sy = q < 2 ? 1.f : -1.f;
      ++index;  // The exact axis-aligned start precedes the sampled points.
      for (double angle = q * half_pi + 0.01; angle < (q + 1) * half_pi;
           angle += 0.01) {
        ASSERT_LT(index, view.header.point_count);
        float x =
            sx * 60 * std::pow(std::abs(std::cos(angle)), 2. / exponent[0]);
        float y =
            sy * 40 * std::pow(std::abs(std::sin(angle)), 2. / exponent[1]);
        EXPECT_NEAR(view.points[index].x, x, 0.00001f);
        EXPECT_NEAR(view.points[index].y, y, 0.00001f);
        ++index;
      }
      ++index;  // The exact endpoint is preserved independently of sampling.
    }
    EXPECT_EQ(index, view.header.point_count);
  }
}
TEST(PathTest, EllipseAndSuperEllipseEndpoints) {
  auto ellipse = Path::MakeEllipse(20, 10);
  ASSERT_TRUE(ellipse);
  auto e = std::get<PathView>(ellipse->GetSourceView());
  EXPECT_EQ(e.points[0].x, 20);
  EXPECT_EQ(e.points[0].y, 0);
  EXPECT_EQ(e.points[3].x, 0);
  EXPECT_EQ(e.points[3].y, 10);
  EXPECT_EQ(e.points[6].x, -20);
  EXPECT_EQ(e.points[9].y, -10);
  EXPECT_EQ(e.commands[e.header.command_count - 1], PathVerb::kClose);
  auto super = Path::MakeSuperEllipse(20, 10, 4, 3);
  ASSERT_TRUE(super);
  auto s = std::get<PathView>(super->GetSourceView());
  EXPECT_EQ(s.points[0].x, 20);
  EXPECT_EQ(s.points[0].y, 0);
  EXPECT_EQ(s.points[s.header.point_count - 1].x, 20);
  EXPECT_EQ(s.points[s.header.point_count - 1].y, 0);
  EXPECT_FALSE(Path::MakeSuperEllipse(1, 1, 0, 2));
  EXPECT_EQ(std::get<PathView>(Path::MakeEllipse(0, 10)->GetSourceView())
                .header.width,
            0);
}
}  // namespace lynx::gfx
