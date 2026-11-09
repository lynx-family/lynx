// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/clip_path_resolver.h"

#include <limits>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx::tasm {
namespace {
ResolvedClipPath Resolve(const char* css, float w = 100, float h = 200,
                         float scale = 1) {
  CSSParserConfigs config;
  StyleMap parsed;
  if (!UnitHandler::Process(kPropertyIDClipPath, lepus::Value(css), parsed,
                            config)) {
    ADD_FAILURE() << "CSS parse failed: " << css;
    return {};
  }
  starlight::ComputedCSSStyle style{scale, 1.f};
  style.SetValue(kPropertyIDClipPath, parsed.at(kPropertyIDClipPath));
  return ResolveClipPath(style.GetClipPath(), w, h, scale);
}
void ExpectInvalidClip(const ResolvedClipPath& clip) {
  EXPECT_EQ(clip.status, ClipPathStatus::kInvalid);
  ASSERT_TRUE(clip.path);
  const auto& header =
      std::get<gfx::PathView>(clip.path->GetSourceView()).header;
  EXPECT_EQ(header.kind, gfx::PathKind::kRect);
  EXPECT_EQ(header.width, 0);
  EXPECT_EQ(header.height, 0);
  EXPECT_EQ(clip.x, 0);
  EXPECT_EQ(clip.y, 0);
}
}  // namespace
TEST(ClipPathResolverTest, CirclePercentageDiagonalAndPlacement) {
  auto p = Resolve("circle(50% at 25% 75%)");
  ASSERT_EQ(p.status, ClipPathStatus::kValid);
  ASSERT_TRUE(p.path);
  auto view = std::get<gfx::PathView>(p.path->GetSourceView());
  ASSERT_EQ(view.header.point_count, 13u);
  EXPECT_NEAR(view.points[0].x, 79.05694, 1e-3);
  EXPECT_NEAR(view.points[3].y, 79.05694, 1e-3);
  EXPECT_EQ(p.x, 25);
  EXPECT_EQ(p.y, 150);
  EXPECT_TRUE(p.depends_on_width);
  EXPECT_TRUE(p.depends_on_height);
}
TEST(ClipPathResolverTest, AbsoluteCircleAndCalcEllipse) {
  auto p = Resolve("circle(10px at 20px 30px)");
  ASSERT_TRUE(p.path);
  EXPECT_FALSE(p.depends_on_width);
  EXPECT_FALSE(p.depends_on_height);
  EXPECT_EQ(p.x, 20);
  EXPECT_EQ(p.y, 30);
  auto e = Resolve("ellipse(calc(10px + 25%) 10px at 0px 0px)");
  ASSERT_EQ(e.status, ClipPathStatus::kValid);
  auto view = std::get<gfx::PathView>(e.path->GetSourceView());
  ASSERT_EQ(view.header.point_count, 13u);
  EXPECT_EQ(view.points[0].x, 35);
  EXPECT_EQ(view.points[3].y, 10);
  EXPECT_TRUE(e.depends_on_width);
  EXPECT_FALSE(e.depends_on_height);
}

TEST(ClipPathResolverTest, SameTypedStyleResolvesAgainstMultipleBoxes) {
  CSSParserConfigs config;
  StyleMap parsed;
  ASSERT_TRUE(UnitHandler::Process(
      kPropertyIDClipPath,
      lepus::Value("ellipse(calc(25% + 10px) 10px at 50% 20px)"), parsed,
      config));
  starlight::ComputedCSSStyle style{1.f, 1.f};
  style.SetValue(kPropertyIDClipPath, parsed.at(kPropertyIDClipPath));
  const auto* shape = style.GetClipPath();
  auto first = ResolveClipPath(shape, 100, 200, 1);
  auto second = ResolveClipPath(shape, 300, 400, 1);
  ASSERT_EQ(first.status, ClipPathStatus::kValid);
  ASSERT_EQ(second.status, ClipPathStatus::kValid);
  EXPECT_EQ(std::get<gfx::PathView>(first.path->GetSourceView()).points[0].x,
            35);
  EXPECT_EQ(std::get<gfx::PathView>(second.path->GetSourceView()).points[0].x,
            85);
  EXPECT_EQ(first.x, 50);
  EXPECT_EQ(second.x, 150);
  EXPECT_EQ(first.y, 20);
  EXPECT_EQ(second.y, 20);
  EXPECT_TRUE(second.depends_on_width);
  EXPECT_FALSE(second.depends_on_height);
  EXPECT_EQ(style.GetClipPath(), shape);
  EXPECT_EQ(shape->RadiusX().NumericLength().GetPercentagePart(), 25);
}

TEST(ClipPathResolverTest, ZeroPercentageDoesNotAddSizeDependencies) {
  for (const char* css : {"ellipse(0% 10px at 0% 0%)",
                          "ellipse(calc(0% + 10px) 10px at 0px 0px)"}) {
    auto clip = Resolve(css);
    EXPECT_EQ(clip.status, ClipPathStatus::kValid);
    EXPECT_FALSE(clip.depends_on_width);
    EXPECT_FALSE(clip.depends_on_height);
  }
}

TEST(ClipPathResolverTest, InsetRectAndAsymmetricRoundedCorners) {
  auto p = Resolve("inset(10% 20px 30px 5%)");
  ASSERT_TRUE(p.path);
  const auto& h = std::get<gfx::PathView>(p.path->GetSourceView()).header;
  EXPECT_EQ(p.x, 5);
  EXPECT_EQ(p.y, 20);
  EXPECT_EQ(h.width, 75);
  EXPECT_EQ(h.height, 150);
  EXPECT_TRUE(p.depends_on_width);
  EXPECT_TRUE(p.depends_on_height);
  auto rounded =
      Resolve("inset(10px round 4px 8px 12px 16px / 2px 6px 10px 14px)");
  ASSERT_TRUE(rounded.path);
  const auto& r = std::get<gfx::PathView>(rounded.path->GetSourceView()).header;
  EXPECT_EQ(r.kind, gfx::PathKind::kRoundedRect);
  const float expected[8] = {4, 2, 8, 6, 12, 10, 16, 14};
  for (int i = 0; i < 8; ++i) EXPECT_EQ(r.radii[i], expected[i]);
}
TEST(ClipPathResolverTest, SuperEllipseAndInsetSuperCorners) {
  auto p = Resolve("super-ellipse(20px 30px 4 3 at 40px 50px)");
  ASSERT_TRUE(p.path);
  EXPECT_EQ(p.x, 40);
  EXPECT_EQ(p.y, 50);
  EXPECT_FALSE(p.depends_on_width);
  EXPECT_FALSE(p.depends_on_height);
  const auto view = std::get<gfx::PathView>(p.path->GetSourceView());
  ASSERT_GE(view.header.point_count, 4u);
  EXPECT_EQ(view.points[0].x, 20);
  EXPECT_EQ(view.points[view.header.point_count / 4].y, 30);
  auto inset = Resolve("inset(10px super-ellipse 4 3 10px)");
  ASSERT_TRUE(inset.path);
  EXPECT_EQ(inset.status, ClipPathStatus::kValid);
  const auto inset_view = std::get<gfx::PathView>(inset.path->GetSourceView());
  EXPECT_EQ(inset_view.header.kind, gfx::PathKind::kCommands);
  ASSERT_GE(inset_view.header.point_count, 4u);
  EXPECT_EQ(inset_view.points[0].x, 80);
  EXPECT_EQ(inset_view.points[inset_view.header.point_count / 4].y, 180);
}
TEST(ClipPathResolverTest, CalculatedInsetCornerRadiiClampAtZero) {
  auto p = Resolve("inset(10px round calc(10% - 20px))", 100, 100);
  ASSERT_TRUE(p.path);
  EXPECT_EQ(p.status, ClipPathStatus::kValid);
  const auto& header = std::get<gfx::PathView>(p.path->GetSourceView()).header;
  EXPECT_EQ(header.width, 80);
  EXPECT_EQ(header.height, 80);
  for (float radius : header.radii) EXPECT_EQ(radius, 0);
}
TEST(ClipPathResolverTest, CalculatedSuperInsetCornerRadiiClampAtZero) {
  auto p = Resolve("inset(10px super-ellipse 4 3 calc(10% - 20px))", 100, 100);
  ASSERT_TRUE(p.path);
  EXPECT_EQ(p.status, ClipPathStatus::kValid);
  const auto view = std::get<gfx::PathView>(p.path->GetSourceView());
  ASSERT_GE(view.header.point_count, 4u);
  EXPECT_EQ(view.points[0].x, 80);
  EXPECT_EQ(view.points[view.header.point_count / 4].x, 0);
  EXPECT_EQ(view.points[view.header.point_count / 4].y, 80);
  EXPECT_EQ(view.points[view.header.point_count / 2].y, 0);
}
TEST(ClipPathResolverTest, InsetOffsetsStayNegativeWhenCornerRadiiClamp) {
  auto p =
      Resolve("inset(-10px round calc(10% - 20px) 6px / 7px calc(10% - 20px))",
              100, 100);
  ASSERT_TRUE(p.path);
  EXPECT_EQ(p.status, ClipPathStatus::kValid);
  EXPECT_EQ(p.x, -10);
  EXPECT_EQ(p.y, -10);
  const auto& header = std::get<gfx::PathView>(p.path->GetSourceView()).header;
  EXPECT_EQ(header.width, 120);
  EXPECT_EQ(header.height, 120);
  const float expected[8] = {0, 7, 6, 0, 0, 7, 6, 0};
  for (int i = 0; i < 8; ++i) EXPECT_EQ(header.radii[i], expected[i]);
}
TEST(ClipPathResolverTest, SvgSourceIsIndependentOfReferenceBox) {
  auto p = Resolve("path(\"m1e2 3 A10 20 45 01-2.5 .3z\")", 100, 200, 2);
  ASSERT_TRUE(p.path);
  ASSERT_EQ(p.status, ClipPathStatus::kValid);
  auto s = std::get<gfx::SvgPathView>(p.path->GetSourceView());
  EXPECT_EQ(s.data, "m1e2 3 A10 20 45 01-2.5 .3z");
  EXPECT_EQ(s.source_to_layout_scale, 2);
  EXPECT_FALSE(p.depends_on_width);
  EXPECT_FALSE(p.depends_on_height);
  EXPECT_TRUE(p.depends_on_source_scale);
}
TEST(ClipPathResolverTest, NoneInvalidAndZeroAreDistinct) {
  EXPECT_EQ(ResolveClipPath(nullptr, 100, 100, 1).status,
            ClipPathStatus::kNoClip);
  auto p = Resolve("ellipse(0px 10px)");
  ASSERT_TRUE(p.path);
  EXPECT_EQ(p.status, ClipPathStatus::kValid);
  EXPECT_EQ(std::get<gfx::PathView>(p.path->GetSourceView()).header.width, 0);
  auto invalid = Resolve("path(\"M0 0\")", 100, 200, 0);
  ASSERT_TRUE(invalid.path);
  EXPECT_EQ(invalid.status, ClipPathStatus::kInvalid);
  EXPECT_EQ(std::get<gfx::PathView>(invalid.path->GetSourceView()).header.width,
            0);
  auto inset = Resolve("inset(60px)", 100, 100);
  ASSERT_TRUE(inset.path);
  EXPECT_EQ(std::get<gfx::PathView>(inset.path->GetSourceView()).header.width,
            0);
}
TEST(ClipPathResolverTest, SvgSyntaxIsDeferredUntilConsumption) {
  auto source = Resolve("path(\"M0 0 L1 1 L2\")");
  ASSERT_TRUE(source.path);
  EXPECT_EQ(source.status, ClipPathStatus::kValid);
  auto view = std::get<gfx::SvgPathView>(source.path->GetSourceView());
  EXPECT_EQ(view.data, "M0 0 L1 1 L2");
  gfx::PathBuilder builder;
  auto consumer = builder.GetConsumer();
  LBSParsePathWithConsumer(view.data.c_str(), &consumer);
  EXPECT_NE(consumer.error, 0);
}
TEST(ClipPathResolverTest, OverflowingGeometryAndPlacementStayInvalid) {
  for (const char* css :
       {"circle(1000% at 0px 0px)", "circle(10px at 1000% 0px)",
        "ellipse(calc(1000% + 1px) 10px at 0px 0px)",
        "ellipse(10px 10px at 0px calc(-1000% + 1px))"}) {
    SCOPED_TRACE(css);
    ExpectInvalidClip(Resolve(css, 1e38f, 1e38f));
  }
}
TEST(ClipPathResolverTest, InvalidResultsRetainDependencies) {
  auto horizontal = Resolve("circle(10px at 1000% 0px)", 1e38f, 1e38f);
  ExpectInvalidClip(horizontal);
  EXPECT_TRUE(horizontal.depends_on_width);
  EXPECT_FALSE(horizontal.depends_on_height);

  auto vertical =
      Resolve("ellipse(10px 10px at 0px calc(-1000% + 1px))", 1e38f, 1e38f);
  ExpectInvalidClip(vertical);
  EXPECT_FALSE(vertical.depends_on_width);
  EXPECT_TRUE(vertical.depends_on_height);

  auto path = Resolve("path(\"M0 0\")", 100, 100, 0);
  ExpectInvalidClip(path);
  EXPECT_TRUE(path.depends_on_source_scale);

  for (float width : {-1.f, std::numeric_limits<float>::infinity()}) {
    auto box = Resolve("circle(50%)", width, 100);
    ExpectInvalidClip(box);
    EXPECT_FALSE(box.depends_on_width);
    EXPECT_FALSE(box.depends_on_height);
    EXPECT_FALSE(box.depends_on_source_scale);
  }

  starlight::BasicShapeData unknown;
  auto invalid = ResolveClipPath(&unknown, 100, 100, 1);
  ExpectInvalidClip(invalid);
  EXPECT_FALSE(invalid.depends_on_width);
  EXPECT_FALSE(invalid.depends_on_height);
  EXPECT_FALSE(invalid.depends_on_source_scale);
}
TEST(ClipPathResolverTest, EmptyInsetDoesNotHideOverflow) {
  for (const char* css :
       {"inset(1000% 0px 0px 0px)", "inset(60% -1000% 60% 0px)"}) {
    SCOPED_TRACE(css);
    ExpectInvalidClip(Resolve(css, 1e38f, 1e38f));
  }
  auto empty = Resolve("inset(60% round calc(-1000% + 1px))", 1e38f, 1e38f);
  ASSERT_TRUE(empty.path);
  EXPECT_EQ(empty.status, ClipPathStatus::kValid);
  const auto& header =
      std::get<gfx::PathView>(empty.path->GetSourceView()).header;
  EXPECT_EQ(header.width, 0);
  EXPECT_EQ(header.height, 0);
}
TEST(ClipPathResolverTest, NegativeCornerOverflowIsNotClampedToZero) {
  for (const char* css : {"inset(0px round calc(-1000% + 1px))",
                          "inset(0px super-ellipse 4 3 calc(-1000% + 1px))"}) {
    SCOPED_TRACE(css);
    ExpectInvalidClip(Resolve(css, 1e38f, 1e38f));
  }
}
}  // namespace lynx::tasm
